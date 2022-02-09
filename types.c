

#include "defs.h"
#include "data.h"
#include "decl.h"

// 型の取り扱い

// 2つのprimitive typeを引数にとり
// 互換性があればtrueを、なければfalseを返す
// 0か、一方がもう一方に合わせるために拡張する必要があればA_WIDEN操作を返す
// onlyrightがtrueであれば左を右へ拡張する
int type_compatible(int *left, int *right, int onlyright)
{
  int leftsize, rightsize;

  // 同じ型であれば互換性がある
  if (*left == *right)
  {
    *left = *right = 0;
    return (1);
  }
  // それぞれの型のサイズを取得
  leftsize = genprimsize(*left);
  rightsize = genprimsize(*right);

  // 0サイズの型はどの型とも互換性がない
  if ((leftsize == 0) || (rightsize == 0))
    return (0);

  // 必要なら型を拡張
  if (leftsize < rightsize)
  {

    *left = A_WIDEN;
    *right = 0;
    return (1);
  }
  if (rightsize < leftsize)
  {
    if (onlyright)
      return (0);
    *left = 0;
    *right = A_WIDEN;
    return (1);
  }
  // 残りは互換性があるものとして扱う
  *left = *right = 0;
  return (1);
}

// Given a primitive type, return
// the type which is a pointer to it
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
