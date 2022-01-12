#include "defs.h"
#include "data.h"
#include "decl.h"

// 宣言のパース

// 変数宣言のパース
void var_declaration(void)
{

  // 'int'トークンの後ろに識別子とセミコロンがあるか確認する
  // Textに識別子名が入る
  // 既知の識別子として加える
  match(T_INT, "int");
  ident();
  addglob(Text);
  genglobsym(Text);
  semi();
}

// 今の所、関数宣言はかなり単純化された文法
// 関数宣言: 'void' identifier '(' ')' 合成ステートメント;

// 単純化された関数の宣言をパース
struct ASTnode *function_declaration(void)
{
  struct ASTnode *tree;
  int nameslot;

  // 'void'、識別子、'(' ')'を探す
  // 今はまだそれらに対して何もしない
  match(T_VOID, "void");
  ident();
  nameslot = addglob(Text);
  lparen();
  rparen();

  // 合成ステートメントのASTツリーを取得
  tree = compound_statement();

  // 関数の名前枠と合成ステートメントのサブツリーを持つ
  // A_FUNCTIONノードを返す
  return (mkastunary(A_FUNCTION, tree, nameslot));
}
