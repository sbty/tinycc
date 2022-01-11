# 11 関数 パート1

関数の実装を包めていきますが、多くのステップに分ける必要があるでしょう。次のような課題が有ります。

- データ型: `char` `int` `long`など
- 関数の戻り値
- 関数への引数の数
- ローカルで関数に渡される変数とグローバル変数の区別

このパートで終わらせるにはあまりに多くの仕事があります。ですからここでは関数の宣言をするところまで進めます。出来上がった実行形式は`main()`関数のみが動くでしょうが、やがて複数の関数のコードを生成できる機能がつくでしょう。

うまく行けばこのコンパイラが認識する言語はCのサブセット相当となり、入力は"本物の"Cコンパイラーで認識できるものとなるでしょう。ですがまだまだです。

## 単純化した関数構文

関数的ななにかをパースできるように、後で間違いなく変更を加えることになります。これを終えると、方や戻り値型、引数といった重要なものに取り掛かることができます。

今の時点ではBNFで言えば次のようになる関数文法を追加するつもりです。

`function_declaration: 'void' identifier '(' ')' compound_statement   ;`

すべての関数は戻り値`void`、引数なしで宣言されます。呼び出すこともできないので`main()`関数のみが実行されます。

新しくキーワード`void`とトークンT_VOIDを追加します。これは簡単です。

## 単純化した関数構文のパース

関数構文はとても単純で、パースに必要な関数をスッキリ小さく記述できます(`decl.c`)。

```c
// 単純化した関数の宣言をパースする
struct ASTnode *function_declaration(void) {
  struct ASTnode *tree;
  int nameslot;

  // 'void'、識別子(関数名)'('と')'を探す
  // いまは見つけても何もしない
  match(T_VOID, "void");
  ident();
  nameslot= addglob(Text);
  lparen();
  rparen();

  // 合成ステートメントのASTツリーを取得
  tree= compound_statement();

  // 関数の名前枠と合成ステートメントのサブツリーを持つA_FUNCTIONノードを返す
  return(mkastunary(A_FUNCTION, tree, nameslot));
}
```

これは構文チェックとASTの構築をしますが、意味解釈のエラーチェックはほとんどしていません。関数が再宣言された場合は今はまだ気が付きません。

## `main()`への変更

上記の関数があれば、`main()`のコードを書き換えを複数の関数を順次パースするよう書き換えることができます。

```c
  scan(&Token);                 // 入力の最初のトークンを取得
  genpreamble();                // プレアンブルを出力
  while (1) {                   // 関数をパース
    tree = function_declaration();
    genAST(tree, NOREG, 0);     // アセンブリコードを出力
    if (Token.token == T_EOF)   // EOFに到達したら終了
      break;
  }
```

`genpostamble()`の関数呼び出しを削除しています。これはその出力が、技術的には`main()`に対して生成されるアセンブリのポストアンブルであったためです。関数の先頭と末尾のコードを生成する関数が必要です。

## 関数の汎用コード生成

A_FUNCTION ASTノードができたので、汎用コード生成`gen.c`に変更を加えたほうがいいでしょう。上記の通り、これは単一の子を持つ単項ASTノードです。

```c
  // Return an 関数の名前枠と合成ステートメントサブツリーを持つA_FUNCTIONノードを返す
  return(mkastunary(A_FUNCTION, tree, nameslot));
```

子は関数の中身となる合成ステートメントが入ったサブツリーを持ちます。合成ステートメントのコードを生成する前に関数の先頭を生成する必要が有ります。そのためのgenA()に追加するコードを示します。

```c
    case A_FUNCTION:
      // コードの前に関数のプレアンブルを生成
      cgfuncpreamble(Gsym[n->v.id].name);
      genAST(n->left, NOREG, n->op);
      cgfuncpostamble();
      return (NOREG);
```

## x86-64コード生成

次は関数にコードを生成してスタックとフレームポインタをセットし、関数の末尾で差し戻し(undo)たら、関数の呼び出し元へ返します。

このコードは`cgpreamble()`と`cgpostamble()`にすでに有りますが、`cgpreamble()`には`printint()`のアセンブリコードも有ります。そのため、これらのアセンブリコードのスニペットは`cg.c`の新しい関数へと分離する必要が有ります。

```c
// アセンブリプレアンブルを出力
void cgpreamble() {
  freeall_registers();
  // printint()のコードのみを出力
}

// 関数プレアンブルを出力
void cgfuncpreamble(char *name) {
  fprintf(Outfile,
          "\t.text\n"
          "\t.globl\t%s\n"
          "\t.type\t%s, @function\n"
          "%s:\n" "\tpushq\t%%rbp\n"
          "\tmovq\t%%rsp, %%rbp\n", name, name, name);
}

// 関数ポストアンブルを出力
void cgfuncpostamble() {
  fputs("\tmovl $0, %eax\n" "\tpopq     %rbp\n" "\tret\n", Outfile);
}
```

## 関数生成の機能をテスト

新しいテストプログラム`tests/input08`は(printを除けば)Cに近づいてきました。

```c
void main()
{
  int i;
  for (i= 1; i <= 10; i= i + 1) {
    print i;
  }
}
```

`make test8`でテストを行うと

```bash
cc -o comp1 -g cg.c decl.c expr.c gen.c main.c misc.c scan.c
    stmt.c sym.c tree.c
./comp1 tests/input08
cc -o out out.s
./out
1
2
3
4
5
6
7
8
9
10
```

アセンブリコードは前回のforループで生成されたものと同じなので触れません。

しかし、言語は合成ステートメントの前に関数宣言が必要なので、これまでのテスト入力ファイルには`void main()`を入れてきました。

テストプログラム`tests/input09`には2つの関数宣言があります。コンパイラは各関数のアセンブリコードを生成しますが、現段階では2つ目の関数を実行することはできません。

## まとめ

言語へ関数を追加するいいスタートが切れました。とりあえずかなり単純化された関数宣言のみです。

次回はコンパイラへ型の追加を始めます。
