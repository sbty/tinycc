#include "defs.h"
#include "data.h"
#include "decl.h"

// Miscellaneous functions

//
// 現在のトークンがtであるか確認して次のトークンを取得する
// tでなければエラー
void match(int t, char *what)
{
  if (Token.token == t)
  {
    scan(&Token);
  }
  else
  {
    fatals("想定", what);
  }
}

// セミコロンか調べて次のトークンを取得
void semi(void)
{
  match(T_SEMI, ";");
}

// '{'と照合して次のトークンを取得
void lbrace(void)
{
  match(T_LBRACE, "{");
}

// '}'と照合して次のトークンを取得
void rbrace(void)
{
  match(T_RBRACE, "}");
}

// '('と照合して次のトークンを取得
void lparen(void)
{
  match(T_LPAREN, "(");
}

// ')'と照合して次のトークンを取得
void rparen(void)
{
  match(T_RPAREN, ")");
}

// 識別子を調べて次のトークンを取得
void ident(void)
{
  match(T_IDENT, "identifier");
}

// エラーを出力
void fatal(char *s)
{
  fprintf(stderr, "%s on line %d\n", s, Line);
  exit(1);
}

void fatals(char *s1, char *s2)
{
  fprintf(stderr, "%s:%s on line %d\n", s1, s2, Line);
  exit(1);
}

void fatald(char *s, int d)
{
  fprintf(stderr, "%s:%d on line %d\n", s, d, Line);
  exit(1);
}

void fatalc(char *s, int c)
{
  fprintf(stderr, "%s:%c on line %d\n", s, c, Line);
  exit(1);
}
