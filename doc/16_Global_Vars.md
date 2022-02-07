# パート16: 正しいグローバル変数の宣言

ポインタにオフセットを追加する問題について検討すると約束しましたが、先に考える必要が有ります。そこでグローバル変数の宣言を関数宣言の外に移動させることにしました。実際のところ、
変数宣言のパースを関数内に残したままです。将来ローカル変数宣言に変更するつもりだからです。

また、同じ型の変数を同時に複数宣言できるよう、文法を拡張したいです。

```c
int x, y, z;
```

## 追加するBNF文法

関数と変数両方のグローバル宣言のためにBNF文法を追加します。

```c
 global_declarations : global_declarations
      | global_declaration global_declarations
      ;

 global_declaration: function_declaration | var_declaration ;

 function_declaration: type identifier '(' ')' compound_statement   ;

 var_declaration: type identifier_list ';'  ;

 type: type_keyword opt_pointer  ;

 type_keyword: 'void' | 'char' | 'int' | 'long'  ;

 opt_pointer: <empty> | '*' opt_pointer  ;

 identifier_list: identifier | identifier ',' identifier_list ;
```

`function_declaration`と`global_declaration`はどちらも`type`で始まります。これは`type_keyword`であり、0個以上の'*'トークンがつく`opt_pointer`が後ろに続きます。その後ろの`function_declaration`と`global_declaration`には1つの識別子が必ず続きます。

しかし、型の後ろには`var_declaration`には`identifier_list`が続き、これは','トークンで区切られた1つ以上の識別子です。`var_declaration`も','トークンで終わる必要が有りますが、`function_declaration`は`compound_statement`で終わり、`;`がありません。

## 追加するトークン

','文字のためにT_COMMAトークンを`scan.c`に用意しました。

## `decl.c`の変更点

上記のBNF構文を再帰降下関数群へと変換するのですが、ループを使えるため、再帰の一部を内部ループへと持っていきます。

### `global_declarations()`

グローバル宣言は1つに限らないため、ループさせることで1つずつパースできます。トークンがなくなったらループを抜けます。

```c
// 1つ以上の変数または関数のグローバル宣言をパースする。
void global_declarations(void) {
  struct ASTnode *tree;
  int type;

  while (1) {

    // 型と識別子の後に読み込みを行い、関数宣言の'('か、
    // 変数宣言の','または';'を確認する必要がある。
    // グローバル変数Textはident()の呼び出しで埋められている。
    type = parse_type();
    ident();
    if (Token.token == T_LPAREN) {

      // 関数宣言をパースしアセンブリコードを生成
      tree = function_declaration(type);
      genAST(tree, NOREG, 0);
    } else {

      // グローバル変数の宣言をパース
      var_declaration(type);
    }

    // EOFまで来たら抜ける
    if (Token.token == T_EOF)
      break;
  }
}
```

今の所、とりあえずグローバル変数と関数しかないので、型と最初の識別子のスキャンをここで行っています。それから次のトークンを見ていきます。'('であれば`function_declaration()`を呼び出し、そうでなければ`var_declaration()`とみなします。両方の関数に型を渡します。

`function_declaration()`からASTツリーを受け取っているので、ASTツリーからのコードをすぐに生成できます。このコードは`main()`にあったものですがここへ持ってきました。`main()`は`global_declaration()`の呼び出しだけとなっています。

```c
  scan(&Token);                 // 入力から最初のトークンを取得
  genpreamble();                // プレアンブルを出力
  global_declarations();        // グローバル宣言をパース
  genpostamble();               // ポストアンブルを出力
```

### `var_declaration()`

関数のパースはこれまでとほぼ同じですが、型と識別子のスキャンが別の場所で行われることと、型を引数として受け取るところが違います。

変数のパースからも型と識別子をスキャンするコードがなくなっています。識別子をグローバルシンボルに加え、そのためのアセンブリコードを生成すればいいのです。ですがそうするとループを追加する必要があります。後ろに','がついていた場合、同じ型を持つ次の識別子を取得するためにループの先頭に戻ります。そして';'がついていた場合、変数宣言の終わりとなります。

```c
// 複数の変数宣言をパースする。
// 識別子はスキャンされており、型情報を持っている。
void var_declaration(int type) {
  int id;

  while (1) {
    // グローバル変数Textには識別子の名前が入っている。
    // 既知の識別子として登録。
    // さらにその場所をアセンブリで生成する。
    id = addglob(Text, type, S_VARIABLE, 0);
    genglobsym(id);

    // 次のトークンがセミコロンであればスキップしてリターン。
    if (Token.token == T_SEMI) {
      scan(&Token);
      return;
    }
    // 次のトークンがコンマであればスキップして識別子を取得、
    // ループの先頭へ戻る。
    if (Token.token == T_COMMA) {
      scan(&Token);
      ident();
      continue;
    }
    fatal("識別子の後ろに , も ; もありません");
  }
}
```

## ローカル変数もどき

`var_declaration()`は複数の変数宣言をパースできるようになりましたが、型と最初の識別子が事前にスキャンされている必要が有ります。

そこで`stmt.c`の`single_statement()`内での`var_declaration()`の呼び出しを残しました。将来、ローカル変数の宣言に対応するためこれを修正することになるでしょう。ですが今はとりあえず、次のサンプルプログラムでの全変数はグローバルとします。

```c
int   d, f;
int  *e;

int main() {
  int a, b, c;
  b= 3; c= 5; a= b + c * 10;
  printint(a);

  d= 12; printint(d);
  e= &d; f= *e; printint(f);
  return(0);
}
```

## 変更点のテスト

上記のコードは`tests/input16.c`です。いつもどおりテストします。

```bash
$ make test16
cc -o comp1 -g -Wall cg.c decl.c expr.c gen.c main.c misc.c scan.c
      stmt.c sym.c tree.c types.c
./comp1 tests/input16.c
cc -o out out.s lib/printint.c
./out
53
12
12
```

## まとめ

次回はポインタにオフセットを追加する問題に取り組みます。
