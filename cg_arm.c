
#include "defs.h"
#include "data.h"
#include "decl.h"

// Raspberry PiでのARMv6対応コードジェネレータ

// 利用可能なレジスタとその名前
static int freereg[4];
static char *reglist[4] = {"r4", "r5", "r6", "r7"};

// すべてのレジスタを利用可能にする
void freeall_registers(void)
{
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// 空いているレジスタを確保する。レジスタ番号を返す。
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
  fatal("レジスタがありません");
  return (NOREG);
}

// 利用可能なレジスタ一覧にレジスタを返す。
// 一覧に残っていないか確認する
static void free_register(int reg)
{
  if (freereg[reg] != 0)
    fatald("レジスタの開放に失敗しました", reg);
  freereg[reg] = 1;
}

// メモリに大きな整数リテラルを保存する必要が有る。
// それらのリストを持っておき、ポストアンブルでの出力に備える
#define MAXINTS 1024
int Intlist[MAXINTS];
static int Intslot = 0;

// .L3ラベルからの大きな整数リテラルのオフセットを決める。
// 整数がリストになければ加える。
static void set_int_offset(int val)
{
  int offset = -1;

  // すでにリストにないか確認
  for (int i = 0; i < Intslot; i++)
  {
    if (Intlist[i] == val)
    {
      offset = 4 * i;
      break;
    }
  }

  // リストになければ加える
  if (offset == -1)
  {
    offset = 4 * Intslot;
    if (Intslot == MAXINTS)
      fatal("set_int_offset():intスロットが足りません");
    Intlist[Intslot++] = val;
  }
  // このオフセットでL3を読み込む
  fprintf(Outfile, "\tldr\tr3, .L3+%d\n", offset);
}

// アセンブリのプレアンブルを出力
void cgpreamble()
{
  freeall_registers();
  fputs("\t.text\n", Outfile);
}

// アセンブリのポストアンブルを出力
void cgpostamble()
{

  // グローバル変数を書き出す
  fprintf(Outfile, ".L2:\n");
  for (int i = 0; i < Globs; i++)
  {
    if (Gsym[i].stype == S_VARIABLE)
      fprintf(Outfile, "\t.word %s\n", Gsym[i].name);
  }

  // 整数リテラルを書き出す
  fprintf(Outfile, ".L3:\n");
  for (int i = 0; i < Intslot; i++)
  {
    fprintf(Outfile, "\t.word %d\n", Intlist[i]);
  }
}

// 関数プレアンブルを書き出す
void cgfuncpreamble(int id)
{
  char *name = Gsym[id].name;
  fprintf(Outfile,
          "\t.text\n"
          "\t.globl\t%s\n"
          "\t.type\t%s, \%%function\n"
          "%s:\n"
          "\tpush\t{fp, lr}\n"
          "\tadd\tfp, sp, #4\n"
          "\tsub\tsp, sp, #8\n"
          "\tstr\tr0, [fp, #-8]\n",
          name, name, name);
}

// 関数ポストアンブルを書き出す
void cgfuncpostamble(int id)
{
  cglabel(Gsym[id].endlabel);
  fputs("\tsub\tsp, fp, #4\n"
        "\tpop\t{fp, pc}\n"
        "\t.align\t2\n",
        Outfile);
}

// 整数リテラル値をレジスタに読み込ませる。
// レジスタ番号を返す。
int cgloadint(int value, int type)
{
  // 新規にレジスタを取得
  int r = alloc_register();

  // リテラル地が小さければ1命令で実行
  if (value <= 1000)
    fprintf(Outfile, "\tmov\t%s, #%d\n", reglist[r], value);
  else
  {
    set_int_offset(value);
    fprintf(Outfile, "\tldr\t%s, [r3]\n", reglist[r]);
  }
  return (r);
}

// .L2ラベルからの変数のオフセットを決める。
// 効率的ではないコード。
static void set_var_offset(int id)
{
  int offset = 0;
  // シンボルテーブルをidについて走査する。
  // Find S_VARIABLEsを見つけて変数にたどり着くまで4を加算

  for (int i = 0; i < id; i++)
  {
    if (Gsym[i].stype == S_VARIABLE)
      offset += 4;
  }
  // このオフセットでr3を読み込む
  fprintf(Outfile, "\tldr\tr3, .L2+%d\n", offset);
}

// 変数の値をレジスタへ読み込む。
// レジスタ番号を返す。
int cgloadglob(int id)
{
  // 新規にレジスタを取得
  int r = alloc_register();

  // 変数へのオフセットを取得
  set_var_offset(id);

  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tldrb\t%s, [r3]\n", reglist[r]);
    break;
  default:
    fprintf(Outfile, "\tldr\t%s, [r3]\n", reglist[r]);
    break;
  }
  return (r);
}

// 2つのレジスタを加算して結果が入ったレジスタ番号を返す。
int cgadd(int r1, int r2)
{
  fprintf(Outfile, "\tadd\t%s, %s, %s\n", reglist[r2], reglist[r1],
          reglist[r2]);
  free_register(r1);
  return (r2);
}

// 1つ目のレジスタから2つ目のレジスタを減算する。
// 結果が入ったレジスタ番号を返す。
int cgsub(int r1, int r2)
{
  fprintf(Outfile, "\tsub\t%s, %s, %s\n", reglist[r1], reglist[r1],
          reglist[r2]);
  free_register(r2);
  return (r1);
}

// 2つのレジスタをかけ合わせて結果が入ったレジスタ番号を返す。
int cgmul(int r1, int r2)
{
  fprintf(Outfile, "\tmul\t%s, %s, %s\n", reglist[r2], reglist[r1],
          reglist[r2]);
  free_register(r1);
  return (r2);
}

// 1つ目のレジスタを2つ目のレジスタで割る。
// 結果が入ったレジスタ番号を返す。
int cgdiv(int r1, int r2)
{

  // 割り算を行うには: r1 は被除数、r2 は除数が入る。
  // 商はr1に入る
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r1]);
  fprintf(Outfile, "\tmov\tr1, %s\n", reglist[r2]);
  fprintf(Outfile, "\tbl\t__aeabi_idiv\n");
  fprintf(Outfile, "\tmov\t%s, r0\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

// 与えられた引数でprintint()を呼び出す
void cgprintint(int r)
{
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r]);
  fprintf(Outfile, "\tbl\tprintint\n");
  fprintf(Outfile, "\tnop\n");
  free_register(r);
}

// 与えられたレジスタから1つの引数を伴う関数を呼び出す。
// 結果が入ったレジスタを返す。
int cgcall(int r, int id)
{
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r]);
  fprintf(Outfile, "\tbl\t%s\n", Gsym[id].name);
  fprintf(Outfile, "\tmov\t%s, r0\n", reglist[r]);
  return (r);
}

// 定数量レジスタを左へシフト
int cgshlconst(int r, int val)
{
  fprintf(Outfile, "\tlsl\t%s, %s, #%d\n", reglist[r], reglist[r], val);
  return (r);
}
// レジスタの値を変数に保存
int cgstorglob(int r, int id)
{

  // 変数へのオフセットを取得
  set_var_offset(id);

  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tstrb\t%s, [r3]\n", reglist[r]);
    break;
  case P_INT:
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tstr\t%s, [r3]\n", reglist[r]);
    break;
  default:
    fatald("cgloadglob:型が不正です", Gsym[id].type);
  }
  return (r);
}

// 型サイズの配列がP_XXXの形で並んでいる。
// 0 はサイズなし。
static int psize[] = {0, 0, 1, 4, 4, 4, 4, 4};

// P_XXXの方の値を引数に、基本型のサイズをバイトで返す。
int cgprimsize(int type)
{
  // 型が有効か調べる
  if (type < P_NONE || type > P_LONGPTR)
    fatal("cgprimsize()型が不正です");
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
  default:
    fatald("cgglobsym: 型サイズが不明です", typesize);
  }
}

// 比較命令のリスト
// ASTの並びは: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] =
    {"moveq", "movne", "movlt", "movgt", "movle", "movge"};

// 反転ジャンプ命令のリスト
// ASTの並びは: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] =
    {"movne", "moveq", "movge", "movle", "movgt", "movlt"};

// 2つのレジスタを比較して真であれば値をセット
int cgcompare_and_set(int ASTop, int r1, int r2)
{

  // AST操作の範囲をチェック
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("cgcompare_and_set()不正なAST操作です");

  fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  fprintf(Outfile, "\t%s\t%s, #1\n", cmplist[ASTop - A_EQ], reglist[r2]);
  fprintf(Outfile, "\t%s\t%s, #0\n", invcmplist[ASTop - A_EQ], reglist[r2]);
  fprintf(Outfile, "\tuxtb\t%s, %s\n", reglist[r2], reglist[r2]);
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
  fprintf(Outfile, "\tb\tL%d\n", l);
}

// 反転分岐命令のリスト
// ASTの並びは: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *brlist[] = {"bne", "beq", "bge", "ble", "bgt", "blt"};

// 2つのレジスタを比較して偽であればジャンプ
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{

  // AST操作の範囲をチェック
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("cgcompare_and_set()不正なAST操作です");

  fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  fprintf(Outfile, "\t%s\tL%d\n", brlist[ASTop - A_EQ], label);
  freeall_registers();
  return (NOREG);
}

// 拡張前から拡張後へレジスタの値を拡張する。
// 新しい値が入ったレジスタを返す。
// this new value
int cgwiden(int r, int oldtype, int newtype)
{
  // 今は何もしない
  return (r);
}

// 関数から値を返すコードを生成
void cgreturn(int reg, int id)
{
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[reg]);
  cgjump(Gsym[id].endlabel);
}

// Generate code to load the address of a global
// identifier into a variable. Return a new register
int cgaddress(int id)
{
  // Get a new register
  int r = alloc_register();

  // Get the offset to the variable
  set_var_offset(id);
  fprintf(Outfile, "\tmov\t%s, r3\n", reglist[r]);
  return (r);
}

// Dereference a pointer to get the value it
// pointing at into the same register
int cgderef(int r, int type)
{
  switch (type)
  {
  case P_CHARPTR:
    fprintf(Outfile, "\tldrb\t%s, [%s]\n", reglist[r], reglist[r]);
    break;
  case P_INTPTR:
    fprintf(Outfile, "\tldr\t%s, [%s]\n", reglist[r], reglist[r]);
    break;
  case P_LONGPTR:
    fprintf(Outfile, "\tldr\t%s, [%s]\n", reglist[r], reglist[r]);
    break;
  }
  return (r);
}
