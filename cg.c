
#include "defs.h"
#include "data.h"
#include "decl.h"

// Code Generator for x86-64

// 利用できるレジスタ一覧とその名前
static int freereg[4];
static char *reglist[4] = {
    "%r8",
    "%r9",
    "%r10",
    "%r11"};

static char *breglist[4] = {
    "%r8b",
    "%r9b",
    "%r10b",
    "%r11b"};

static char *dreglist[4] = {
    "%r8d",
    "%r9d",
    "%r10d",
    "%r11d"};

// すべてのレジスタを利用可能にする
void freeall_registers(void)
{
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// 利用可能なレジスタを確保する。レジスタ番号を返す。
// 利用可能なレジスタがなければ終了
static int alloc_register(void)
{
  for (int i = 0; i < 4; i++)
  {
    if (freereg[i])
    {
      freereg[i] = 0;
      return (i);
    }
  }
  fatal("利用可能なレジスタがありません");
  return (NOREG);
}

// 利用可能なレジスタのリストへ(利用中の)レジスタを返す
//
static void free_register(int reg)
{
  if (freereg[reg] != 0)
  {
    fatald("レジスタの開放に失敗しました", reg);
  }
  freereg[reg] = 1;
}

// アセンブリのプレアンブルを出力
void cgpreamble()
{
  freeall_registers();
  fputs("\t.text\n", Outfile);
}

void cgpostamble()
{
}

// 関数のプレアンブルを出力
void cgfuncpreamble(int id)
{
  char *name = Gsym[id].name;
  fprintf(Outfile,
          "\t.text\n"
          "\t.globl\t%s\n"
          "\t.type\t%s, @function\n"
          "%s:\n"
          "\tpushq\t%%rbp\n"
          "\tmovq\t%%rsp, %%rbp\n",
          name, name, name);
}

// 関数のポストアンブルを出力
void cgfuncpostamble(int id)
{
  cglabel(Gsym[id].endlabel);
  fputs("\tpopq %rbp\n"
        "\tret\n",
        Outfile);
}

// 整数リテラル値をレジスタに読み込む
// レジスタ番号を返す
int cgloadint(int value, int type)
{
  // 新規にレジスタを確保();
  int r = alloc_register();

  // 初期化コードを出力
  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
  return (r);
}

// 変数からレジスタに値を読み込む
// レジスタ番号を返す
int cgloadglob(int id)
{
  // 新規にレジスタを取得
  int r = alloc_register();

  // 初期化コードを出力:P_CHARかP_INT
  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", Gsym[id].name,
            reglist[r]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovzbl\t%s(\%%rip), %s\n", Gsym[id].name,
            reglist[r]);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
    break;
  default:
    fatald("cgloadglob:型が不正です", Gsym[id].type);
  }
  return (r);
}

// 2つのレジスタを足し合わせて、
// 結果の入ったレジスタ番号を返す
int cgadd(int r1, int r2)
{
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

// 1つ目のレジスタから2つめのレジスタを引いて
// 結果の入ったレジスタ番号を返す
int cgsub(int r1, int r2)
{
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
}

// 2つのレジスタを掛け合わせて、
// 結果の入ったレジスタ番号を返す
int cgmul(int r1, int r2)
{
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

// 1つ目のレジスタを2つめのレジスタで割って
// 結果の入ったレジスタ番号を返す
int cgdiv(int r1, int r2)
{
  fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
  fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

// printint()に引数を渡して呼び出し
void cgprintint(int r)
{
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\tprintint\n");
  free_register(r);
}

// 引数のレジスタから1つの引数を伴う関数を呼び出す。
// 結果が入ったレジスタを返す
int cgcall(int r, int id)
{
  // 新規にレジスタを取得
  int outr = alloc_register();
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\t%s\n", Gsym[id].name);
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
  free_register(r);
  return (outr);
}

// 定数量レジスタを左へシフト
int cgshlconst(int r, int val)
{
  fprintf(Outfile, "\tsalq\t$%d, %s\n", val, reglist[r]);
  return (r);
}

// レジスタの値を変数に保存
int cgstorglob(int r, int id)
{
  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, %s(\%%rip)\n", breglist[r],
            Gsym[id].name);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %s(\%%rip)\n", dreglist[r],
            Gsym[id].name);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], Gsym[id].name);
    break;
  default:
    fatald("cgloadglob:不正な型です", Gsym[id].type);
  }
  return (r);
}

// P_XXXの並びで型のサイズが入った配列
// 0 はサイズなし
static int psize[] = {
    0,
    0,
    1,
    4,
    8,
    8,
    8,
    8};

// P_XXX型の値を引数に基本型のサイズをバイトで返す
int cgprimsize(int type)
{
  // 型が有効か調べる
  if (type < P_NONE || type > P_LONGPTR)
    fatal("cgprimsize():不正な型です");
  return (psize[type]);
}

// グローバルシンボルを生成
void cgglobsym(int id)
{
  int typesize;
  // 型のサイズを取得
  typesize = cgprimsize(Gsym[id].type);

  fprintf(Outfile, "\t.data\n"
                   "\t.globl\t%s\n",
          Gsym[id].name);

  switch (typesize)
  {
  case 1:
    fprintf(Outfile, "%s:\t.byte\t0\n", Gsym[id].name);
    break;
  case 4:
    fprintf(Outfile, "%s:\t.long\t0\n", Gsym[id].name);
    break;
  case 8:
    fprintf(Outfile, "%s:\t.quad\t0\n", Gsym[id].name);
    break;
  default:
    fatald("cgglobsym:型のサイズが不明です ", typesize);
  }
}

// 比較命令のリスト
// ASTの並び: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] =
    {"sete", "setne", "setl", "setg", "setle", "setge"};

// 2つのレジスタを比較して真であればセット
int cgcompare_and_set(int ASTop, int r1, int r2)
{

  // AST操作の範囲をチェック
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("cgcompare_and_set()内での不正なAST操作");

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
  free_register(r1);
  return (r2);
}

// ラベルを生成
void cglabel(int l)
{
  fprintf(Outfile, "L%d:\n", l);
}

// ラベルへのジャンプを生成
void cgjump(int l)
{
  fprintf(Outfile, "\tjmp\tL%d\n", l);
}

// ひっくり返したジャンプ命令のリスト
// ASTのならび: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = {"jne", "je", "jge", "jle", "jg", "jl"};

// 2つのレジスタを比較して偽ならジャンプ
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{

  // AST操作の範囲をチェック
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("cgcompare_and_set()内での不正なAST操作");

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
  freeall_registers();
  return (NOREG);
}

// レジスタの値を拡張前から拡張後の新しい型へと拡張する
// 値を格納したレジスタを返す
int cgwiden(int r, int oldtype, int newtype)
{
  // なにもしない
  return (r);
}

// 関数から値を返すコードを生成
void cgreturn(int reg, int id)
{
  // 関数の型に応じてコードを生成
  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[reg]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[reg]);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
    break;
  default:
    fatald("cgreturn:関数の型が不正です", Gsym[id].type);
  }
  cgjump(Gsym[id].endlabel);
}

// グローバル識別子のアドレスを変数へ読み込む
// コードを生成する。レジスタ番号を返す。
int cgaddress(int id)
{
  int r = alloc_register();

  fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", Gsym[id].name, reglist[r]);
  return (r);
}

// ポインタを間接参照して
// 同じレジスタを指す値を取得する
int cgderef(int r, int type)
{
  switch (type)
  {
  case P_CHARPTR:
    fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  case P_INTPTR:
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  }
  return (r);
}
