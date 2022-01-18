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

  // voidはどの型とも互換性がない
  if ((*left == P_VOID) || (*right == P_VOID))
    return (0);

  // 同じ型であれば互換性がある
  if (*left == *right)
  {
    *left = *right = 0;
    return (1);
  }

  // 必要であれば P_CHAR を P_INT へ拡張
  if ((*left == P_CHAR) && (*right == P_INT))
  {
    *left = A_WIDEN;
    *right = 0;
    return (1);
  }
  if ((*left == P_INT) && (*right == P_CHAR))
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
