#include "defs.h"
#include "data.h"
#include "decl.h"

// ステートメントのパース

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
  int reg;

  // printを最初のトークンとしてチェック
  match(T_PRINT, "print");

  // 続く式をパースしアセンブリを生成
  tree = binexpr(0);

  // print ASTツリーを作成
  tree = mkastunary(A_PRINT, tree, 0);

  // セミコロンがあるか調べる
  semi();
  return (tree);
}

// assignment_statement: identifier '=' expression ';'   ;
static struct ASTnode *assignment_statement(void)
{
  struct ASTnode *left, *right, *tree;
  int id;

  // 識別子があるか調べる
  ident();

  // 定義されていたら葉ノードを作る
  if ((id = findglob(Text)) == -1)
  {
    fatals("宣言されていない変数", Text);
  }
  right = mkastleaf(A_LVIDENT, id);

  // イコールがあるか調べる
  match(T_ASSIGN, "=");

  // 続く式をパース
  left = binexpr(0);

  // 代入のASTを作る
  tree = mkastnode(A_ASSIGN, left, NULL, right, 0);

  // セミコロンが後ろにあるか調べる
  semi();
  return (tree);
}

// if_statement: if_head
//      |        if_head 'else' compound_statement
//      ;
//
// if_head: 'if' '(' true_false_expression ')' compound_statement  ;

// else句を含む可能性のあるif文をパース
struct ASTnode *if_statement(void)
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
  return (mkastnode(A_IF, condAST, trueAST, falseAST, 0));
}

// while_statement: 'while' '(' true_false_expression ')' compound_statement  ;
//
// Parse a WHILE statement
// and return its AST
struct ASTnode *while_statement(void)
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
  return (mkastnode(A_WHILE, condAST, NULL, bodyAST, 0));
}

// 合成ステートメントをパスしそのASTを返す
struct ASTnode *compound_statement(void)
{
  struct ASTnode *left = NULL;
  struct ASTnode *tree;

  // '{'を確認
  lbrace();

  while (1)
  {
    switch (Token.token)
    {
    case T_PRINT:
      tree = print_statement();
      break;
    case T_INT:
      var_declaration();
      tree = NULL; // ここではASTを作らない
      break;
    case T_IDENT:
      tree = assignment_statement();
      break;
    case T_IF:
      tree = if_statement();
      break;
    case T_WHILE:
      tree = while_statement();
      break;

    case T_RBRACE:
      // '}'を見つけたら終わりまでスキップしてASTを返す
      rbrace();
      return (left);
    default:
      fatald("構文エラー、token", Token.token);
    }

    // 新規ツリーが左が空であれば左に保存し、
    // からでなければ左ツリーと新規ツリーを結合する
    if (tree)
    {
      if (left == NULL)
        left = tree;
      else
        left = mkastnode(A_GLUE, left, NULL, tree, 0);
    }
  }
}
