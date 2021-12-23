#include "defs.h"
#include "data.h"
#include "decl.h"

// シンボルテーブル関数

static int Globs = 0; // グローバルシンボルスロットの次に空いている位置

// シンボルsがグローバルシンボルテーブルにあるか判断する
// 見つかればその位置、見つからなければ-1を返す
int findglob(char *s)
{
  int i;

  for (i = 0; i < Globs; i++)
  {
    if (*s == *Gsym[i].name && !strcmp(s, Gsym[i].name))
      return (i);
  }
  return (-1);
}

// 新規のグローバルシンボルスロットの位置を取得
// 空きがなければ終了
static int newglob(void)
{
  int p;

  if ((p = Globs++) >= NSYMBOLS)
    fatal("グローバルシンボルが多すぎます");
  return (p);
}

// グローバルシンボルをシンボルテーブルに追加する
// シンボルテーブルのスロット番号を返す
int addglob(char *name)
{
  int y;

  // シンボルテーブルにすでにあれば既存のスロット番号を返す
  if ((y = findglob(name)) != -1)
    return (y);

  // なければ新規スロットを取得して格納し、そのスロット番号を返す
  y = newglob();
  Gsym[y].name = strdup(name);
  return (y);
}
