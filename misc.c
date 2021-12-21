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
    printf("%s ではありませんでした line %d\n", what, Line);
    exit(1);
  }
}

// セミコロンか調べて次のトークンを取得
void semi(void)
{
  match(T_SEMI, ";");
}
