#include "defs.h"
#include "data.h"
#include "decl.h"

// ステートメントのパース

// Prototypes
static struct ASTnode *single_statement(void);

// compound_statement:          // empty, i.e. no statement
//      |      statement
//      |      statement statements
//      ;
//
// statement: print_statement
//      |     declaration
//      |     assignment_statement
//      |     if_statement
//      |     while_statement
//      ;
//
// print_statement: 'print' expression ';'  ;

static struct ASTnode *print_statement(void)
{
  struct ASTnode *tree;
  int lefttype, righttype;
  int reg;

  // printを最初のトークンとしてチェック
  match(T_PRINT, "print");

  // 続く式をパースしアセンブリを生成
  tree = binexpr(0);

  // 2つの型に互換性があるか確認
  lefttype = P_INT;
  righttype = tree->type;
  if (!type_compatible(&lefttype, &righttype, 0))
    fatal("型に互換性がありません");

  // 要求があれば型を拡張
  if (righttype)
    tree = mkastunary(righttype, P_INT, tree, 0);

  // printASTツリーを出力
  tree = mkastunary(A_PRINT, P_NONE, tree, 0);

  return (tree);
}

// assignment_statement: identifier '=' expression ';'   ;
static struct ASTnode *assignment_statement(void)
{
  struct ASTnode *left, *right, *tree;
  int lefttype, righttype;
  int id;

  // 識別子があるか調べる
  ident();

  // 定義されていたら葉ノードを作る
  if ((id = findglob(Text)) == -1)
  {
    fatals("宣言されていない変数", Text);
  }
  right = mkastleaf(A_LVIDENT, Gsym[id].type, id);

  // イコールがあるか調べる
  match(T_ASSIGN, "=");

  // 続く式をパース
  left = binexpr(0);

  // 2つの型に互換性があるか調べる
  lefttype = left->type;
  righttype = right->type;
  if (!type_compatible(&lefttype, &righttype, 1))
    fatal("Incompatible types");

  // 要求があれば左を拡張
  if (lefttype)
    left = mkastunary(lefttype, right->type, left, 0);

  // 代入のASTを作る
  tree = mkastnode(A_ASSIGN, P_INT, left, NULL, right, 0);

  return (tree);
}

// if_statement: if_head
//      |        if_head 'else' compound_statement
//      ;
//
// if_head: 'if' '(' true_false_expression ')' compound_statement  ;

// else句を含む可能性のあるif文をパース
static struct ASTnode *if_statement(void)
{
  struct ASTnode *condAST, *trueAST, *falseAST = NULL;

  // 'if' '('があるか確認
  match(T_IF, "if");
  lparen();

  // ')'がついた、続く式をパース
  // ツリーの操作が比較である確認をする
  condAST = binexpr(0);

  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("不正な比較演算子");
  rparen();

  // 合成ステートメント用ASTを取得
  trueAST = compound_statement();

  // elseがあるならスキップして合成ステートメントのASTを取得
  if (Token.token == T_ELSE)
  {
    scan(&Token);
    falseAST = compound_statement();
  }
  // このステートメントのASTを作成して返す
  return (mkastnode(A_IF, P_NONE, condAST, trueAST, falseAST, 0));
}

// while_statement: 'while' '(' true_false_expression ')' compound_statement  ;
//
// Parse a WHILE statement
// and return its AST
static struct ASTnode *while_statement(void)
{
  struct ASTnode *condAST, *bodyAST;

  // 'while'と'('があるか確認
  match(T_WHILE, "while");
  lparen();

  // 続く式と後ろについた')'をパース
  // ツリーの操作が比較であることを確認
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("不正な比較操作");
  rparen();

  // 合成ステートメントのASTを取得
  bodyAST = compound_statement();

  // このステートメントのASTを作成して返す
  return (mkastnode(A_WHILE, P_NONE, condAST, NULL, bodyAST, 0));
}

// for_statement: 'for' '(' preop_statement ';'
//                          true_false_expression ';'
//                          postop_statement ')' compound_statement  ;
//
// preop_statement:  statement          (for now)
// postop_statement: statement          (for now)
//
// FORステートメントをパースして
// そのASTを返す
static struct ASTnode *for_statement(void)
{
  struct ASTnode *condAST, *bodyAST;
  struct ASTnode *preopAST, *postopAST;
  struct ASTnode *tree;

  // 'for'と'('があるか確認
  match(T_FOR, "for");
  lparen();

  // pre_opステートメントと';'を取得
  preopAST = single_statement();
  semi();

  // 条件式と';'を取得
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("不正な比較演算子です");
  semi();

  // post_opステートメントと')'を取得
  postopAST = single_statement();
  rparen();

  // ループの中身となるcompoundステートメントを取得
  bodyAST = compound_statement();

  // 現段階では4つのサブツリーはNULLであってはならない。
  // 後で空が混在しているときの意味解釈に手を加える。

  // compoundステートメントとpostopツリーを結合
  tree = mkastnode(A_GLUE, P_NONE, bodyAST, NULL, postopAST, 0);

  // 条件式とループ本文でWHILEループを作る
  tree = mkastnode(A_WHILE, P_NONE, condAST, NULL, tree, 0);

  // preopツリーをA_WHILEツリーに結合
  return (mkastnode(A_GLUE, P_NONE, preopAST, NULL, tree, 0));
}

// single statementをパースして
// そのASTを返す
static struct ASTnode *single_statement(void)
{
  switch (Token.token)
  {
  case T_PRINT:
    return (print_statement());
  case T_CHAR:
  case T_INT:
    var_declaration();
    return (NULL); // No AST generated here
  case T_IDENT:
    return (assignment_statement());
  case T_IF:
    return (if_statement());
  case T_WHILE:
    return (while_statement());
  case T_FOR:
    return (for_statement());
  default:
    fatald("構文エラー token", Token.token);
  }
}

// 合成ステートメントをパースしそのASTを返す
struct ASTnode *compound_statement(void)
{
  struct ASTnode *left = NULL;
  struct ASTnode *tree;

  // '{'を確認
  lbrace();

  while (1)
  {
    // single statementをパース
    tree = single_statement();
    // 後ろにセミコロンがつかないといけないステートメントか調べる
    if (tree != NULL && (tree->op == A_PRINT || tree->op == A_ASSIGN))
      semi();

    // それぞれのツリーに対し、左が空であれば左に保存し、
    // そうでなければ新規ツリーと左を結合する
    if (tree != NULL)
    {
      if (left == NULL)
        left = tree;
      else
        left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, 0);
    }
    // '}'を見つけたらそこまでスキップし
    // ASTを返す
    if (Token.token == T_RBRACE)
    {
      rbrace();
      return (left);
    }
  }
}
