# ARMアセンブリコードの生成

 Raspberry Pi 4に乗っているARM CPU向けに対応していきます。

## 大きな違い

まずARMはRISC CPUであり、x86-64はCISC CPUです。ARMはx86-64に比べるとアドレッシング・モードが少ないです。またARMアセンブリコードの生成の際に興味深い制約が有ります。そこで大きな違いからはじめ、主な類似点は後回しにします。

### ARMレジスタ

ARMはx86-64よりも多くのレジスタを持ちます。そこで今回は4つのレジスタ`r4`、`r5`、`r6`、`r7`を確保することにします。`r0`、`r3`には他の役割があるので後で説明します。

### グローバル変数の対応

x86-64ではグローバル変数の宣言に必要なものは以下だけです。

```assembly
        .comm   i,4,4        # int 変数
        .comm   j,1,1        # char 変数
```

これ以降、これらの変数への読み込みや保存は容易です。

```assembly
        movb    %r8b, j(%rip)    # jに保存
        movl    %r8d, i(%rip)    # iに保存
        movzbl  i(%rip), %r8     # iから読み込み
        movzbq  j(%rip), %r8     # jから読み込み
```

ARMでは、プログラムのポストアンブルですべてのグローバル変数を手動で確保する必要が有ります。

```assembly
        .comm   i,4,4
        .comm   j,1,1
...
.L2:
        .word i
        .word j
```

これらにアクセスするには、各変数のアドレスが入ったレジスタを読み込み、それからそのアドレスから2つ目のレジスタを読み込む必要が有ります。

```assembly
        ldr     r3, .L2+0
        ldr     r4, [r3]        # iを読み込み
        ldr     r3, .L2+4
        ldr     r4, [r3]        # jを読み込み
```

変数への保存は同様に

```assembly
        mov     r4, #20
        ldr     r3, .L2+4
        strb    r4, [r3]        # i= 20
        mov     r4, #10
        ldr     r3, .L2+0
        str     r4, [r3]        # j= 10
```

.wordのテーブルを生成するため、`cgpostamble()`に以下のコードを追加しています。

```c
  // グローバル変数の書き出し
  fprintf(Outfile, ".L2:\n");
  for (int i = 0; i < Globs; i++) {
    if (Gsym[i].stype == S_VARIABLE)
      fprintf(Outfile, "\t.word %s\n", Gsym[i].name);
  }
```

これはそれぞれのグローバル変数に対して`.L2`からのオフセットを決めなければならないということです。KISS原則に従い、手動でそれぞれのオフセットを毎回計算し、変数のアドレスを`r3`に読み込もうと思います。本来はオフセットを一度計算したらどこかに保存しておくべきです。

```c
// .L2ラベルからの変数のオフセットを決定する
// 現状非効率なコード
static void set_var_offset(int id) {
  int offset = 0;
  // シンボルテーブルのidを見ていく
  // S_VARIABLEを探し変数が見つかるまで4を足していく

  for (int i = 0; i < id; i++) {
    if (Gsym[i].stype == S_VARIABLE)
      offset += 4;
  }
  // オフセットと合わせてr3を読み込む
  fprintf(Outfile, "\tldr\tr3, .L2+%d\n", offset);
}
```

### intリテラルの読み込み

読み込み命令時における整数リテラルのサイズは11ビットに制限され、符号付きの値だと思います。したがって、大きな整数リテラルを単一の命令に入れることはできません。解決案は整数値を変数などのメモリに保存することです。そうすることでそれまで使ってきた整数値のリストを維持します。ポストアンブルで`.L3`ラベルの後ろにこれらを出力します。そして変数と同様にこのリストを見て`.L3`ラベルから任意の変数へのオフセットを決定します。

```c
// メモリに大きな整数リテラル値を保存する必要がある。
// ポストアンブルで出力される可能性のあるリストを保持する。
#define MAXINTS 1024
int Intlist[MAXINTS];
static int Intslot = 0;

// 大きな整数リテラルの.L3ラベルからのオフセットを決める。
// 整数がリストになければ追加する。
static void set_int_offset(int val) {
  int offset = -1;

  // すでにあるか確認
  for (int i = 0; i < Intslot; i++) {
    if (Intlist[i] == val) {
      offset = 4 * i;
      break;
    }
  }

  // リストになければ追加
  if (offset == -1) {
    offset = 4 * Intslot;
    if (Intslot == MAXINTS)
      fatal("set_int_offset()でintスロットがありません");
    Intlist[Intslot++] = val;
  }
  // このオフセットでr3を読み込む
  fprintf(Outfile, "\tldr\tr3, .L3+%d\n", offset);
}
```

### 関数プレアンブル

関数プレアンブルを示しますが、各命令がしていることを完全に理解していません。以下は`int main(int x)`の場合です。

```asseembly
  .text
  .globl        main
  .type         main, %function
  main:         push  {fp, lr}          # フレームとスタックポインタを保存
                add   fp, sp, #4        # スタックポインタにsp+4を加算
                sub   sp, sp, #8        # スタックポインタを8下げる
                str   r0, [fp, #-8]     # 引数をローカル変数として保存するか？
```

以下は単一の戻り値のための関数ポストアンブルです。

```assembly
                sub   sp, fp, #4        # ???
                pop   {fp, pc}          # フレームとスタックポインタを取り出す。
```

### 0か1を返すことの比較

x86-64では`sete`のような比較が真であるかに基づいて0か1をレジスタにセットする命令が有ります。ですがその場合、残りのレジスタを`movzbq`を使って0で埋める必要が有ります。ARMでは別々に2つの命令を実行し、条件が真か偽かでレジスタに値をセットします。

```assembly
                moveq r4, #1            # 値が等しければr4に1をセット
                movne r4, #0            # 値が等しくなければr4に0をセット
```

##
