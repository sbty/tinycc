

#include "defs.h"
#include "data.h"
#include "decl.h"

// 型の取り扱い

// サイズが何であれ型がintであればtrue、
// そうでなければfalseを返す。
int inttype(int type)
{
  if (type == P_CHAR || type == P_INT || type == P_LONG)
    return (1);
  return (0);
}

// 型がポインタ型であればtrueを返す
int ptrtype(int type)
{
  if (type == P_VOIDPTR || type == P_CHARPTR ||
      type == P_INTPTR || type == P_LONGPTR)
    return (1);
  return (0);
}

// 基本型を引数に、その型を指すポインタを返す
int pointer_to(int type)
{
  int newtype;
  switch (type)
  {
  case P_VOID:
    newtype = P_VOIDPTR;
    break;
  case P_CHAR:
    newtype = P_CHARPTR;
    break;
  case P_INT:
    newtype = P_INTPTR;
    break;
  case P_LONG:
    newtype = P_LONGPTR;
    break;
  default:
    fatald("pointer_to: 型を認識できません", type);
  }
  return (newtype);
}

// 基本型のポインタを引数に取り、
// そのポインタが指す型を返す。
int value_at(int type)
{
  int newtype;
  switch (type)
  {
  case P_VOIDPTR:
    newtype = P_VOID;
    break;
  case P_CHARPTR:
    newtype = P_CHAR;
    break;
  case P_INTPTR:
    newtype = P_INT;
    break;
  case P_LONGPTR:
    newtype = P_LONG;
    break;
  default:
    fatald("value_at: 認識できない型です。", type);
  }
  return (newtype);
}

// ASTツリーと変換先の型を引数にとり
// 引数の型と互換性を持つよう
// 拡張や拡縮によってツリーを修正する。
// 修正がなかった場合は元のツリーを、修正した場合はそのツリーを、
// ツリーが引数の型と互換性がなかった場合はNULLを返す。
// このツリーが二項演算子の一部となるのであれば
// AST操作は0以外になる。
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op)
{
  int ltype;
  int lsize, rsize;

  ltype = tree->type;

  // int型の大きさを比較
  if (inttype(ltype) && inttype(rtype))
  {

    // 同じ型であれば何もしない
    if (ltype == rtype)
      return (tree);

    // 各型のサイズを取得
    lsize = genprimsize(ltype);
    rsize = genprimsize(rtype);

    // ツリーのサイズが大きすぎる場合
    if (lsize > rsize)
      return (NULL);

    // 右側へ拡張
    if (rsize > lsize)
      return (mkastunary(A_WIDEN, rtype, tree, 0));
  }
  // 左側にあるポインタの処理
  if (ptrtype(ltype))
  {
    // 右側にあるものと同じ型であり
    // 二項演算でなければOK
    if (op == 0 && ltype == rtype)
      return (tree);
  }
  // A_ADD または A_SUBTRACT操作に対してのみスケール可能
  if (op == A_ADD || op == A_SUBTRACT)
  {

    // 左がint、右がポインタであり
    // 元の型のサイズが1より大きければ
    // 左をスケール
    if (inttype(ltype) && ptrtype(rtype))
    {
      rsize = genprimsize(value_at(rtype));
      if (rsize > 1)
        return (mkastunary(A_SCALE, rtype, tree, rsize));
    }
  }
  // ここに到達したら型に互換性はない
  return (NULL);
}
