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
    fprintf(stderr, "構文エラー line %d\n", Line);
    exit(1);
  }
}

// トークンをAST操作に変換
int arithop(int tok)
{
  switch (tok)
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
    fprintf(stderr, "不明なトークンです。arithop() line %d\n", Line);
    fprintf(stderr, "%d %c\n", tok, tok);
    exit(1);
  }
}

// ルートが２項演算子であるASTツリーを返す。
struct ASTnode *binexpr(void)
{
  struct ASTnode *n, *left, *right;
  int nodetype;

  // 左の整数リテラルを取得
  // 同時に次のトークンも取得
  left = primary();

  // トークンが残ってなければ左ノードだけを返す。
  if (Token.token == T_EOF)
    return (left);

  // トークンをノード型へ変換
  nodetype = arithop(Token.token);

  // 次のトークンへ
  scan(&Token);

  // 再帰的に右側のツリーを取得
  right = binexpr();

  // サブツリーを含めたツリーを構築
  n = mkastnode(nodetype, left, right, 0);
  return (n);
}
