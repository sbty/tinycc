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
