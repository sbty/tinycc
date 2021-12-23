#include "defs.h"
#include "data.h"
#include "decl.h"

// 宣言のパース

// 変数宣言のパース
void var_declaration(void)
{

  // 'int'トークンの後ろに識別子とセミコロンがあるか確認する
  // Textに識別子名が入る
  // 既知の識別子に加える
  match(T_INT, "int");
  ident();
  addglob(Text);
  genglobsym(Text);
  semi();
}
