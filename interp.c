#include "defs.h"
#include "data.h"
#include "decl.h"

// ASTツリーの解釈

// AST操作のリスト
static char *ASTop[] = {"+", "-", "*", "/"};

// 与えられたASTの演算子を解釈し最終値を返す
int interpretAST(struct ASTnode *n)
{
  int leftval, rightval;

  // 左右のサブツリーの値を取得
  if (n->left)
    leftval = interpretAST(n->left);
  if (n->right)
    rightval = interpretAST(n->right);

  //デバッグ用出力
  /*
  if (n->op == A_INTLIT)
    printf("int %d\n", n->intvalue);
  else
    printf("%d %s %d\n", leftval, ASTop[n->op], rightval);
*/

  switch (n->op)
  {
  case A_ADD:
    return (leftval + rightval);
  case A_SUBTRACT:
    return (leftval - rightval);
  case A_MULTIPLY:
    return (leftval * rightval);
  case A_DIVIDE:
    return (leftval / rightval);
  case A_INTLIT:
    return (n->intvalue);
  default:
    fprintf(stderr, "不明なAST操作fです。%d\n", n->op);
    exit(1);
  }
}
