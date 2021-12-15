#include "defs.h"
#include "data.h"
#include "decl.h"

// 主な要素をパースしてそれらを表すASTノードを返す。
static struct ASTnode *primary(void)
{
  struct ASTnode *n;

  // INTLITトークンが来たら葉ASTノードを作り次のノードをスキャンする
  // そうでなければ他の型は何が来ても構文エラー。
  switch (Token.token)
  {
  case T_INTLIT:
    n = mkastleaf(A_INTLIT, Token.intvalue);
    scan(&Token);
    return (n);
  default:
    fprintf(stderr, "構文エラー　line %d, token %d\n", Line, Token.token);
    exit(1);
  }
}

//二項演算子をAST操作に変換
static int arithop(int tok)
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
    fprintf(stderr, "不明なトークン line %d, token %d\n", Line, Token.token);
    exit(1);
  }
}

struct ASTnode *additive_expr(void);

// ルートが二項演算子'*'か'/'であるASTツリーを返す
struct ASTnode *multiplicative_expr(void)
{
  struct ASTnode *left, *right;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンも取得
  left = primary();

  // トークンが残ってなければ左のノードを返す
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // トークンが'*'か'/'である限りループする
  while ((tokentype == T_STAR) || (tokentype == T_SLASH))
  {
    // 次の整数リテラルを取得
    scan(&Token);
    right = primary();

    // 左の整数リテラルを取得
    left = mkastnode(arithop(tokentype), left, right, 0);

    // 現在のトークンの詳細を更新
    // トークンが残ってなければ左ノードを返す
    tokentype = Token.token;
    if (tokentype == T_EOF)
      break;
  }

  // 作ったツリーを返す
  return (left);
}

// // ルートが二項演算子'+'か'-'であるASTツリーを返す
struct ASTnode *additive_expr(void)
{
  struct ASTnode *left, *right;
  int tokentype;

  // 現在より優先度が高いものとして左のサブツリーを取得する
  left = multiplicative_expr();

  // トークンが残ってなければ左ノードを返す
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // '+'か'-'のトークンをキャッシュ

  // 優先度が同じである限りループ
  while (1)
  {
    // 次の整数リテラルを取得
    scan(&Token);

    // より高い優先度として右のサブツリーを取得
    right = multiplicative_expr();

    // ２つのサブツリーをより低い優先度のオペレータと結合
    left = mkastnode(arithop(tokentype), left, right, 0);

    // 同じ優先度の次のトークンを取得
    tokentype = Token.token;
    if (tokentype == T_EOF)
      break;
  }

  // 作成したツリーを返す
  return (left);
}

struct ASTnode *binexpr(int n)
{
  return (additive_expr());
}
