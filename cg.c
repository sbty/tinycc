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
  fprintf(stderr, "利用可能なレジスタがありません\n");
  exit(1);
}

// 利用可能なレジスタのリストへ(利用中の)レジスタを返す
//
static void free_register(int reg)
{
  if (freereg[reg] != 0)
  {
    fprintf(stderr, "レジスタの開放に失敗しました %d\n", reg);
    exit(1);
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
      "\n"
      "\t.globl\tmain\n"
      "\t.type\tmain, @function\n"
      "main:\n"
      "\tpushq\t%rbp\n"
      "\tmovq	%rsp, %rbp\n",
      Outfile);
}

// アセンブリポストアンブルを出力
void cgpostamble()
{
  fputs(
      "\tmovl	$0, %eax\n"
      "\tpopq	%rbp\n"
      "\tret\n",
      Outfile);
}

// 整数リテラル値をレジスタに読み込む
// レジスタ番号を返す
int cgload(int value)
{
  // 新規にレジスタを確保();
  int r = alloc_register();

  // 初期化コードを出力
  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
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
