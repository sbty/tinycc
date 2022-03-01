#include "defs.h"
#include "data.h"
#include "decl.h"

// 汎用コードジェネレータ

// 新しいラベル番号を生成して返す
int genlabel(void)
{
  static int id = 1;
  return (id++);
}

// if文とオプションのelse句のコードを生成する
static int genIF(struct ASTnode *n)
{
  int Lfalse, Lend;

  // 2つのラベルを生成する。1つはfalse合成ステートメント、
  // もう1つはif文全体の終わりへのラベル。
  // else句がなければLfalseが終了ラベルとなる
  Lfalse = genlabel();
  if (n->right)
    Lend = genlabel();

  // falseラベルへジャンプするコードが後ろにつく、
  // 条件式を生成する。
  // Lfalseラベルをレジスタとして送るインチキ(cheat)をする
  genAST(n->left, Lfalse, n->op);
  genfreeregs();

  // true合成ステートメントを生成
  genAST(n->mid, NOLABEL, n->op);
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
    genAST(n->right, NOLABEL, n->op);
    genfreeregs();
    cglabel(Lend);
  }

  return (NOREG);
}

// whileステートメントと、オプションのelse句の
// コードを生成
static int genWHILE(struct ASTnode *n)
{
  int Lstart, Lend;

  // 開始と終了ラベルを生成
  // スタートラベルを出力
  Lstart = genlabel();
  Lend = genlabel();
  cglabel(Lstart);

  // 後ろに終了ラベルへのジャンプ文がつく
  // 条件分岐を生成。
  // Lfalseラベルをレジスタとして送るインチキをする
  genAST(n->left, Lend, n->op);
  genfreeregs();

  // while本文の合成ステートメントを生成
  genAST(n->right, NOLABEL, n->op);
  genfreeregs();

  // 最後に終了ラベルと条件分岐へ戻る出力
  cgjump(Lstart);
  cglabel(Lend);
  return (NOREG);
}

// ASTと(あれば)前の右辺値を保持するレジスタ、
// 親のAST操作を引数に取り、再帰的に
// アセンブリコードを生成する。
// 作成した木の最終的な値とそのレジスタIDを返す
int genAST(struct ASTnode *n, int label, int parentASTop)
{
  int leftreg, rightreg;

  // 優先して扱う必要のあるASTノード
  switch (n->op)
  {
  case A_IF:
    return (genIF(n));
  case A_WHILE:
    return (genWHILE(n));
  case A_GLUE:
    // 子ステートメントそれぞれと結合しレジスタを開放する
    genAST(n->left, NOLABEL, n->op);
    genfreeregs();
    genAST(n->right, NOLABEL, n->op);
    genfreeregs();
    return (NOREG);
  case A_FUNCTION:
    // コードより先に関数のプレアンブルを生成
    cgfuncpreamble(n->v.id);
    genAST(n->left, NOLABEL, n->op);
    cgfuncpostamble(n->v.id);
    return (NOREG);
  }

  // ここからは汎用ASTノードの操作

  // 左右のサブツリーの値を取得
  if (n->left)
    leftreg = genAST(n->left, NOLABEL, n->op);
  if (n->right)
    rightreg = genAST(n->right, NOLABEL, n->op);

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
    if (parentASTop == A_IF || parentASTop == A_WHILE)
      return (cgcompare_and_jump(n->op, leftreg, rightreg, label));
    else
      return (cgcompare_and_set(n->op, leftreg, rightreg));

  case A_INTLIT:
    return (cgloadint(n->v.intvalue, n->type));
  case A_IDENT:
    // 右辺値または間接参照されているのであれば値を読み込む
    if (n->rvalue || parentASTop == A_DEREF)
      return (cgloadglob(n->v.id));
    else
      return (NOREG);

  case A_ASSIGN:
    // 識別子への代入かポインタの介した代入か
    switch (n->right->op)
    {
    case A_IDENT:
      return (cgstorglob(leftreg, n->right->v.id));
    case A_DEREF:
      return (cgstorderef(leftreg, rightreg, n->right->type));
    default:
      fatald("genAST()でA_ASSIGNEはできません。 op", n->op);
    }
  case A_WIDEN:
    // 子の型を親の型へ拡張
    return (cgwiden(leftreg, n->left->type, n->type));
  case A_RETURN:
    cgreturn(leftreg, Functionid);
    return (NOREG);
  case A_FUNCCALL:
    return (cgcall(leftreg, n->v.id));
  case A_ADDR:
    return (cgaddress(n->v.id));
  case A_DEREF:
    // 右辺値であればポインタの指す値を取得するため間接参照をする
    // そうでなければポインタを介して保存するためにA_ASSIGNを残しておく
    if (n->rvalue)
      return (cgderef(leftreg, n->left->type));
    else
      return (leftreg);
  case A_SCALE:
    // 小さな最適化: スケール値が2の乗数であればシフト演算を使う
    switch (n->v.size)
    {
    case 2:
      return (cgshlconst(leftreg, 1));
    case 4:
      return (cgshlconst(leftreg, 2));
    case 8:
      return (cgshlconst(leftreg, 3));
    default:
      // サイズが入ったレジスタを読み込み、
      // leftregをsize分だけ倍加する
      rightreg = cgloadint(n->v.size, P_INT);
      return (cgmul(leftreg, rightreg));
    }
  default:
    fatald("不明なAST操作です", n->op);
  }
  return (NOREG);
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

void genglobsym(int id)
{
  cgglobsym(id);
}

int genprimsize(int type)
{
  return (cgprimsize(type));
}
