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
