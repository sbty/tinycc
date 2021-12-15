#include "defs.h"
#include "data.h"
#include "decl.h"

// 主要な要素をパースして、ASTノードとして返す
static struct ASTnode *primary(void)
{
  struct ASTnode *n;

  // INTLITトークンであれば葉としてASTノードを作り
  // 次のトークンをスキャンする。それ以外はどのトークンタイプでも構文エラー
  switch (Token.token)
  {
  case T_INTLIT:
    n = mkastleaf(A_INTLIT, Token.intvalue);
    scan(&Token);
    return (n);
  default:
    fprintf(stderr, "構文エラー line %d, token %d\n", Line, Token.token);
    exit(1);
  }
}

// トークンをAST操作に変換
int arithop(int tokentype)
{
  switch (tokentype)
  {
  case T_PLUS:
    return (A_ADD);
  case T_MINUS:
    return (A_SUBTRACT);
  case T_STAR:
    return (A_MULTIPLY);
  case T_SLASH:
    return (A_DIVIDE);
  default:
    fprintf(stderr, "不明なトークンです。arithop() line %d, token %d\n", Line, tokentype);
    exit(1);
  }
}

// 各トークンのオペレータ優先順位
static int OpPrec[] = {0, 10, 10, 20, 20, 0};

// ２項演算子であることを確認してその優先順位を返す
static int op_precedence(int tokentype)
{
  int prec = OpPrec[tokentype];
  if (prec == 0)
  {
    fprintf(stderr, "構文エラー line %d, token %d\n", Line, tokentype);
    exit(1);
  }
  return (prec);
}

// ルートが２項演算子であるASTツリーを返す。
// 引数ptpは１つ前のトークンの優先順位
struct ASTnode *binexpr(int ptp)
{
  struct ASTnode *n, *left, *right;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンも取得
  left = primary();

  // トークンが残ってなければ左ノードだけを返す。
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // 1つ前のトークンよりも現在のトークンの
  // 優先順位が高い限りループする。
  while (op_precedence(tokentype) > ptp)
  {
    //次の整数リテラルを取得
    scan(&Token);

    // 再帰的に現在のトークンの優先順位をbinexpr()に渡して
    // サブツリーを構築
    right = binexpr(OpPrec[tokentype]);

    // サブツリーを結合する。
    // 同時にトークンをAST操作に変換
    left = mkastnode(arithop(tokentype), left, right, 0);

    // 現在のトークンの詳細を更新
    // トークンが残ってなければ左ノードを返す
    tokentype = Token.token;
    if (tokentype == T_EOF)
      return (left);
  }
  //優先順位が同じか低いトークンが出る前までのツリーを返す。
  return (left);
}
