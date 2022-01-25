# 本物のコンパイラ

今回はこれまで作ってきたインタラプタを、x86-64アセンブラコードを生成するものに置き換えていきます。

## インタプリタの修正

インタラプタは次のようになっていました。

```c
int interpretAST(struct ASTnode *n) {
  int leftval, rightval;

  if (n->left) leftval = interpretAST(n->left);
  if (n->right) rightval = interpretAST(n->right);

  switch (n->op) {
    case A_ADD:      return (leftval + rightval);
    case A_SUBTRACT: return (leftval - rightval);
    case A_MULTIPLY: return (leftval * rightval);
    case A_DIVIDE:   return (leftval / rightval);
    case A_INTLIT:   return (n->intvalue);

    default:
      fprintf(stderr, "不明なAST操作です %d\n", n->op);
      exit(1);
  }
}
```

`interpretAST()関数`は与えられたASTツリーを深さ優先で見ていきます。とにかく左のサブツリーを評価し、それから右のサブツリーへ向かいます。最後に現在のツリーの根本にある操作値(op value)を使ってこれらの子ツリーを最初の操作を操作します。

操作値が４つの算術演算子のいづれかであれば、実行されます。操作値が整数リテラルを示しているのであれば、リテラル値が返ります。

関数はこのツリーの最終的な値を返します。そして再帰であるため、複数のサブツリーから成る、全体のツリーの最終値を計算します。

## アセンブリコード生成への移行

汎用的なアセンブリコードジェネレータを記述していきます。これは順番にCPU固有のコード生成関数のセットを呼び出すものになるでしょう。

`gen.c`の汎用アセンブリコードジェネレータは次のようになります。

```c
// 与えられたASTノードから再帰的にアセンブリコードを生成する
static int genAST(struct ASTnode *n) {
  int leftreg, rightreg;

  // 左右のサブツリーの値を取得
  if (n->left) leftreg = genAST(n->left);
  if (n->right) rightreg = genAST(n->right);

  switch (n->op) {
    case A_ADD:      return (cgadd(leftreg,rightreg));
    case A_SUBTRACT: return (cgsub(leftreg,rightreg));
    case A_MULTIPLY: return (cgmul(leftreg,rightreg));
    case A_DIVIDE:   return (cgdiv(leftreg,rightreg));
    case A_INTLIT:   return (cgload(n->intvalue));

    default:
      fprintf(stderr, "不明なASTオペレータです %d\n", n->op);
      exit(1);
  }
}
```

これまでと同様、深さ優先の木探索を行っています。違いは以下です。

- A_INTLIT: リテラル値でレジスタを読み込む
- ほかオペレータ: 左右の子ツリーの値が入っている２つのレジスタで算術関数を実行する。

値を渡すのではなく、`genAST()`のコードでレジスタ識別子を渡します。例えば`cgload()`はレジスタに値を読み込み、そのレジスタの識別子と読み込まれた値を返します。

`genAST()`自体はその時点のツリーの最終値を保持しているレジスタの識別子を返します。そのためにコードの先頭でレジスタ識別子を取得しています。

```c
  if (n->left) leftreg = genAST(n->left);
  if (n->right) rightreg = genAST(n->right);
```

## genAST()の呼び出し

genAST()は与えられた式から値を計算するだけです。この最終的な計算を出力する必要があります。また生成したアセンブリコードの先頭と末尾を必要なコード(プリアンブル、ポストアンブル)で包んでやる必要もあります。これはgen.cの別な関数でやります。

```c
void generatecode(struct ASTnode *n) {
  int reg;

  cgpreamble();
  reg = genAST(n);
  cgprintint(reg);      // 最終的な値が入ったレジスタをintとして出力
  cgpostamble();
}
```

## x86-64コードジェネレータ

汎用コードジェネレータは不要になりました。実際のアセンブリコード生成を見ていきます。x86-64 CPUをターゲットに考えています。これは最も普及しているlinuxプラットフォームの1つだからです。`cg.c`を見ていきましょう。

### レジスタの確保

どのCPUでもレジスタの数には限りがあります。整数リテラル値を保存し、それらに対する計算を実行するのに必要なレジスタを確保しなければなりません。ですが、使い終えた値は破棄することができるので、そういったレジスタは開放できます。ですからレジスタを別の値に再利用することが可能です。

レジスタの確保を扱う３つの関数を用意します。

- freeall_registers():すべてのレジスタを利用可能とする
- alloc_register(): 空いているレジスタを確保する
- free_register(): 確保しているレジスタを開放する

現時点では、レジスタが足りなくなるとプログラムはクラッシュします。レジスタの空きが足りなくなった場合の処理は後回しとします。

コードは汎用レジスタr0、r1、r2、r3に対して動作します。実際のレジスタ名の文字列テーブルがあります。

```c
static char *reglist[4]= { "%r8", "%r9", "%r10", "%r11" };
```

これによりこれらの関数はCPUアーキテクチャにほとんど依存しません。

### レジスタの読み込み

`cgload()`で行います。レジスタを確保し、`movq`命令でリテラル値を確保したレジスタへ読み込みます。

```c
// レジスタに整数リテラル値を読み込む
// レジスタの数を返す
int cgload(int value) {

  // 新規にレジスタを取得
  int r= alloc_register();

  // 初期化するためのコードを出力
  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
  return(r);
}
```

### 2つのレジスタの加算

`cgadd()`は2つのレジスタ番号を受け取り、それらを加算するコードを生成します。結果は2つのレジスタの一方へ保存され、もう一方は再利用のために開放されます。

```c
// 2つのレジスタを加算し、結果が入ったレジスタ番号を返す
int cgadd(int r1, int r2) {
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return(r2);
}
```

足し算は可換であるため、`r1`を`r2`へ足すのではなく、`r2`を`r1`へ足すこともできます。結果が入っているレジスタの番号が返ります。

### 2つのレジスタの乗算

加算とほとんど同じで操作は可換であるため、どのレジスタを返すことができます。

```c
// 2つのレジスタを乗算し、結果が入ったレジスタ番号を返す
int cgmul(int r1, int r2) {
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return(r2);
}
```

### 2つのレジスタの減算

引き算は非可換であるため、正しく並ばせる必要があります。1番めのレジスタから2番目が引かれるので、結果の入った1番目を返し2番目は開放します。

```c
// 1番目から2番めを引く。結果が入ったレジスタ番号を返す。
int cgsub(int r1, int r2) {
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return(r1);
}
```

### 2つのレジスタの除算

割り算も非可換であるため、引き算と同様です。x86-64では更に複雑になります。`r1`からの割られる数を`%rax`に読み込みます。これを`cqo`で8バイトに拡張します。さらに`idivq`で`r2`の割る数で`%rax`を割り、商を`%rax`に残します。これを`r1`か`r2`にコピーする必要があります。もう一方のレジスタは開放できます。

```c
// 1つ目のレジスタを2つ目のレジスタで割る
// 結果が入ったレジスタ番号を返す
int cgdiv(int r1, int r2) {
  fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
  fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
  free_register(r2);
  return(r1);
}
```

### レジスタの出力

x86-64にはレジスタを10進数で出力する命令はありません。これを解決するため、アセンブリプレアンブルには`printint()`が含まれており、これはレジスタを引数として`printf()`を呼び出し10進で出力する出力することができます。

```c
void cgprintint(int r) {
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\tprintint\n");
  free_register(r);
}
```

linuxのx86-64は関数の最初の引数は`%rdi`に入ると想定しているのでprintint()を呼ぶ前にレジスタを`%rdi`にコピーしています。

## はじめてのコンパイル

x86-64のコードジェネレータはこれくらいでしょう。main()にout.sを出力ファイルとして書き出すためのコードがいくらか追加されています。入力式の結果がインタラプタとアセンブリで同じか確認できるよう、インタラプタを残しています。

input01をコンパイルして実行してみます。

```bash
$ make
cc -o comp1 -g cg.c expr.c gen.c interp.c main.c scan.c tree.c

$ make test
./comp1 input01
15
cc -o out out.s
./out
15
```

最初の15がインタラプタの出力、2つめがアセンブリの出力です。

## アセンブリ出力のテスト

次の入力に対し

```c
2 + 3 * 5 - 8 / 3
```

コメント付きの`out.s`が以下になります。

```assembly
        .text                           # プレアンブルコード
.LC0:
        .string "%d\n"                  # printf()用の"%d\n"
printint:
        pushq   %rbp
        movq    %rsp, %rbp              # フレームポインタをセット
        subq    $16, %rsp
        movl    %edi, -4(%rbp)
        movl    -4(%rbp), %eax          # printint()への引数を取得
        movl    %eax, %esi
        leaq    .LC0(%rip), %rdi        # "%d\n"へのポインタを取得
        movl    $0, %eax
        call    printf@PLT              # printf()の呼び出し
        nop
        leave                           # return
        ret

        .globl  main
        .type   main, @function
main:
        pushq   %rbp
        movq    %rsp, %rbp              # フレームポインタをセット
                                        # プレアンブルコードの終了

        movq    $2, %r8                 # %r8 = 2
        movq    $3, %r9                 # %r9 = 3
        movq    $5, %r10                # %r10 = 5
        imulq   %r9, %r10               # %r10 = 3 * 5 = 15
        addq    %r8, %r10               # %r10 = 2 + 15 = 17
                                        # %r8 and %r9 are now free again
        movq    $8, %r8                 # %r8 = 8
        movq    $3, %r9                 # %r9 = 3
        movq    %r8,%rax
        cqo                             # 被除数を%raxに8バイトで読み込む
        idivq   %r9                     # 3で割る
        movq    %rax,%r8                # 商を%r8に保存、例では2
        subq    %r8, %r10               # %r10 = 17 - 2 = 15
        movq    %r10, %rdi              # 15を%rdiにコピー
        call    printint                # 関数呼び出し

        movl    $0, %eax                # ポストアンブル: exit(0)を呼び出し
        popq    %rbp
        ret
```

これで正規のコンパイラができました。ある言語による入力を受けて、別の言語への変換を出力するプログラムです。

このあと出力を機械語にアセンブルし、サポートライブラリとリンクする必要がありますが、手動であれば今でもできることです。自動化は後ほど行います。

## まとめ

インタプリタから汎用コードジェネレータへの移行は簡単でしたが、実際にアセンブリを生成するコードを記述しなければなりませんでした。それにはどうやってレジスタを確保するか考える必要がありました。今の所は簡素な解決策を用意しています。idivq命令のような、x86-64の奇妙な命令にも対応する必要がありました。

次回はいくつか機能を付け足し、本物のコンパイラに近づけていきます。

### メモ
ニーモニックのsuffix `q`は"quadword"、8バイトを意味します。`movq %rax, %rbx​`は8バイト分`%rax`から`%rbx`へデータをコピー、を意味します。

同様にsuffix `l`は"doubleword"で4バイト、suffix `w`は"word"で2バイト、suffix `b`は"byte"で1バイトをそれぞれ意味します。
