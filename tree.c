#include "defs.h"
#include "data.h"
#include "decl.h"

struct ASTnode *mkastnode(int op, struct ASTnode *left, struct ASTnode *right, int intvalue)
{
  struct ASTnode *n;

  // ASTノードを確保
  n = (struct ASTnode *)malloc(sizeof(struct ASTnode));
  if (n == NULL)
  {
    fprintf(stderr, "メモリが確保できませんでした。mkastnode() \n");
    exit(1);
  }

  n->op = op;
  n->left = left;
  n->right = right;
  n->intvalue = intvalue;
  return (n);
}

// 葉としてASTノードを作成
struct ASTnode *mkastleaf(int op, int intvalue)
{
  return (mkastnode(op, NULL, NULL, intvalue));
}

// １つだけ子を持つ単項ASTノードをつくる
struct ASTnode *mkastunary(int op, struct ASTnode *left, int intvalue)
{
  return (mkastnode(op, left, NULL, intvalue));
}
