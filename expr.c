#include "defs.h"
#include "data.h"
#include "decl.h"

// 式をパース
// 1つの引数を伴う関数呼び出しをパースし
// ASTを返す
struct ASTnode *funccall(void)
{
  struct ASTnode *tree;
  int id;

  // 識別子医が定義されているか調べる
  // それから葉ノードを作る
  if ((id = findglob(Text)) == -1)
  {
    fatals("宣言されていない関数です", Text);
  }
  // '(' を取得
  lparen();

  // 後続の式をパース
  tree = binexpr(0);

  // 関数呼び出しASTノードを作成
  // 関数の戻り値をノードの型として保存
  // ファンクションのシンボルidを記録
  tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);

  // ')' を取得
  rparen();
  return (tree);
}

// 主要な要素をパースして、ASTノードとして返すP
static struct ASTnode *primary(void)
{
  struct ASTnode *n;
  int id;

  // INTLITトークンであれば葉としてASTノードを作り
  // 次のトークンをスキャンする。それ以外はどのトークンタイプでも構文エラー
  switch (Token.token)
  {
  case T_INTLIT:
    // INTLITトークンであればAST葉ノードを作る
    // P_CHARの範囲内であればP_CHARで作る
    if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
      n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
    else
      n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
    break;

  case T_IDENT:
    // これは変数または関数呼び出し
    // 次のトークンをスキャンして判定
    scan(&Token);

    // '(' であれば関数呼び出し
    if (Token.token == T_LPAREN)
      return (funccall());

    // 関数呼び出しでないのでトークンを差し戻す
    reject_token(&Token);

    // 変数が宣言されているか調べる
    id = findglob(Text);
    if (id == -1)
      fatals("不明な変数", Text);

    // AST葉ノードを作る
    n = mkastleaf(A_IDENT, Gsym[id].type, id);
    break;

  default:
    fatald("構文エラー、トークン", Token.token);
  }
  scan(&Token);
  return (n);
}

// 二項演算子トークンをAST操作に変換
// トークンからAST操作は1:1で対応しなければならない
static int arithop(int tokentype)
{
  if (tokentype > T_EOF && tokentype < T_INTLIT)
    return (tokentype);
  fatald("構文エラー、トークン", tokentype);
}

// 各トークンのオペレータ優先順位
static int OpPrec[] = {
    0, 10, 10,       // T_EOF, T_PLUS, T_MINUS
    20, 20,          // T_STAR, T_SLASH
    30, 30,          // T_EQ, T_NE
    40, 40, 40, 40}; // T_LT, T_GT, T_LE, T_GE

// ２項演算子であることを確認してその優先順位を返す
static int op_precedence(int tokentype)
{
  int prec = OpPrec[tokentype];
  if (prec == 0)
    fatald("構文エラー トークン", tokentype);
  return (prec);
}

// ルートが２項演算子であるASTツリーを返す。
// 引数ptpは１つ前のトークンの優先順位
struct ASTnode *binexpr(int ptp)
{
  struct ASTnode *left, *right;
  int lefttype, righttype;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンも取得
  left = primary();

  // セミコロンか')'を見つけたら左ノードだけを返す。
  tokentype = Token.token;
  if (tokentype == T_SEMI || tokentype == T_RPAREN)
    return (left);

  // 1つ前のトークン(ptp)よりも現在のトークンの
  // 優先順位が高い限りループする。
  while (op_precedence(tokentype) > ptp)
  {
    //次の整数リテラルを取得
    scan(&Token);

    // 再帰的に現在のトークンの優先順位をbinexpr()に渡して
    // サブツリーを構築
    right = binexpr(OpPrec[tokentype]);

    // 2つの型に互換性があるか確認
    lefttype = left->type;
    righttype = right->type;
    if (!type_compatible(&lefttype, &righttype, 0))
      fatal("型に互換性がありません");

    // 要求があればどちらか一方を拡張する
    // k型を表す変数はA_WIDEN
    if (lefttype)
      left = mkastunary(lefttype, right->type, left, 0);
    if (righttype)
      right = mkastunary(righttype, left->type, right, 0);

    // サブツリーを結合する。
    // 同時にトークンをAST操作に変換
    left = mkastnode(arithop(tokentype), left->type, left, NULL, right, 0);

    // 現在のトークンの詳細を更新
    // セミコロンか')'を見つけたら左ノードを返す
    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN)
      return (left);
  }
  //優先順位が同じか低いトークンが出る前までのツリーを返す。
  return (left);
}
