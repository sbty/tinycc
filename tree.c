#include "defs.h"
#include "data.h"
#include "decl.h"

// ASTツリー関数

// ASTノードを生成して返す
struct ASTnode *mkastnode(int op, int type, struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right, int intvalue)
{
  struct ASTnode *n;

  // ASTノードを確保
  n = (struct ASTnode *)malloc(sizeof(struct ASTnode));
  if (n == NULL)
  {
    fatal("メモリが確保できませんでした。mkastnode()");
  }

  n->op = op;
  n->type = type;
  n->left = left;
  n->mid = mid;
  n->right = right;
  n->v.intvalue = intvalue;
  return (n);
}

// 葉としてASTノードを作成
struct ASTnode *mkastleaf(int op, int type, int intvalue)
{
  return (mkastnode(op, type, NULL, NULL, NULL, intvalue));
}

// １つだけ子を持つ単項ASTノードをつくる
struct ASTnode *mkastunary(int op, int type, struct ASTnode *left, int intvalue)
{
  return (mkastnode(op, type, left, NULL, NULL, intvalue));
}
