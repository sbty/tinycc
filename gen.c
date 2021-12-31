#include "defs.h"
#include "data.h"
#include "decl.h"

// 汎用コードジェネレータ

// 新しいラベル番号を生成して返す
static int label(void)
{
  static int id = 1;
  return (id++);
}

// if文とオプションのelse句のコードを生成する
static int genIFAST(struct ASTnode *n)
{
  int Lfalse, Lend;

  // 2つのラベルを生成する。1つはfalse合成ステートメント、
  // もう1つはif文全体の終わりへのラベル。
  // else句がなければLfalseが終了ラベルとなる
  Lfalse = label();
  if (n->right)
    Lend = label();

  // falseラベルへジャンプするコードが後ろにつく、
  // 条件式を生成する。
  // Lfalseラベルをレジスタとして送るインチキ(cheat)をする
  genAST(n->left, Lfalse, n->op);
  genfreeregs();

  // true合成ステートメントを生成
  genAST(n->mid, NOREG, n->op);
  genfreeregs();

  // else句があれば終わりまでスキップするジャンプを生成
  if (n->right)
    cgjump(Lend);

  // falseラベル
  cglabel(Lfalse);

  // オプションのelse句:false合成ステートメントと
  // 終了ラベルを生成
  if (n->right)
  {
    genAST(n->right, NOREG, n->op);
    genfreeregs();
    cglabel(Lend);
  }

  return (NOREG);
}

// ASTと(あれば)前の右辺値を保持するレジスタ、
// 親のAST操作を引数に取り、再帰的に
// アセンブリコードを生成する。
// 作成した木の最終的な値とそのレジスタIDを返す
int genAST(struct ASTnode *n, int reg, int parentASTop)
{
  int leftreg, rightreg;

  // 優先して扱う必要のあるASTノード
  switch (n->op)
  {
  case A_IF:
    return (genIFAST(n));
  case A_GLUE:
    // 子ステートメントそれぞれと
    genAST(n->left, NOREG, n->op);
    genfreeregs();
    genAST(n->right, NOREG, n->op);
    genfreeregs();
    return (NOREG);
  }

  // ここからは汎用ASTノードの操作

  // 左右のサブツリーの値を取得
  if (n->left)
    leftreg = genAST(n->left, NOREG, n->op);
  if (n->right)
    rightreg = genAST(n->right, leftreg, n->op);

  switch (n->op)
  {
  case A_ADD:
    return (cgadd(leftreg, rightreg));
  case A_SUBTRACT:
    return (cgsub(leftreg, rightreg));
  case A_MULTIPLY:
    return (cgmul(leftreg, rightreg));
  case A_DIVIDE:
    return (cgdiv(leftreg, rightreg));
  case A_EQ:
  case A_NE:
  case A_LT:
  case A_GT:
  case A_LE:
  case A_GE:
    // 親ASTノードがA_IFであれば、後ろにジャンプがつく比較を
    // 生成する。そうでなければレジスタを比較し、その結果に応じて
    // 0か1をセットする
    if (parentASTop == A_IF)
      return (cgcompare_and_jump(n->op, leftreg, rightreg, reg));
    else
      return (cgcompare_and_set(n->op, leftreg, rightreg));

  case A_INTLIT:
    return (cgloadint(n->v.intvalue));
  case A_IDENT:
    return (cgloadglob(Gsym[n->v.id].name));
  case A_LVIDENT:
    return (cgstorglob(reg, Gsym[n->v.id].name));
  case A_ASSIGN:
    // やることを終えたので結果を返す
    return (rightreg);
  case A_PRINT:
    // 左の子の値を出力しNOREGを返す
    genprintint(leftreg);
    genfreeregs();
    return (NOREG);

  default:
    fatald("不明なAST操作です", n->op);
  }
}

void genpreamble()
{
  cgpreamble();
}
void genpostamble()
{
  cgpostamble();
}
void genfreeregs()
{
  freeall_registers();
}
void genprintint(int reg)
{
  cgprintint(reg);
}

void genglobsym(char *s)
{
  cgglobsym(s);
}
