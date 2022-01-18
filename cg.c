#include "defs.h"
#include "data.h"
#include "decl.h"

// Code Generator

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
  fputs(
      "\t.text\n"
      ".LC0:\n"
      "\t.string\t\"%d\\n\"\n"
      "printint:\n"
      "\tpushq\t%rbp\n"
      "\tmovq\t%rsp, %rbp\n"
      "\tsubq\t$16, %rsp\n"
      "\tmovl\t%edi, -4(%rbp)\n"
      "\tmovl\t-4(%rbp), %eax\n"
      "\tmovl\t%eax, %esi\n"
      "\tleaq	.LC0(%rip), %rdi\n"
      "\tmovl	$0, %eax\n"
      "\tcall	printf@PLT\n"
      "\tnop\n"
      "\tleave\n"
      "\tret\n"
      "\n",
      Outfile);
}

// 関数のプレアンブルを出力
void cgfuncpreamble(char *name)
{
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
void cgfuncpostamble()
{
  fputs("\tmovl $0, %eax\n"
        "\tpopq %rbp\n"
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
  if (Gsym[id].type == P_INT)
    fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
  else
    fprintf(Outfile, "\tmovzbq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
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

// レジスタの値を変数に保存
int cgstorglob(int r, int id)
{
  // P_INTかP_CHARを選ぶ
  if (Gsym[id].type == P_INT)
    fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], Gsym[id].name);
  else
    fprintf(Outfile, "\tmovb\t%s, %s(\%%rip)\n", breglist[r], Gsym[id].name);

  return (r);
}

// グローバルシンボルを生成
void cgglobsym(int id)
{
  // P_INTかP_CHARを選ぶ
  if (Gsym[id].type == P_INT)
    fprintf(Outfile, "\t.comm\t%s,8,8\n", Gsym[id].name);
  else
    fprintf(Outfile, "\t.comm\t%s,1,1\n", Gsym[id].name);
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
