# パート19: 配列パート1

今回はコンパイラに配列を追加していきます。どういった機能を実装するべきか、小さなCプログラムを書いて確認します。

```c
  int ary[5];               // 5つのint要素からなる配列
  int *ptr;                 // intへのポインタ

  ary[3]= 63;               // (左辺値)ary[3]を63にセット
  ptr   = ary;              // ポインタ変数ptrに配列の先頭をセット
  // ary= ptr;              // エラー: 配列型の式への代入
  ptr   = &ary[0];          // 同様にptrに配列の先頭をセット、ary[0]は左辺値
  ptr[4]= 72;               // ptrを配列のように使う。ptr[4]は左辺値
```

配列とポインタは"[]"構文を使って間接参照し、特定の要素にアクセスできるという点では似ています。配列の名前をポインタのように利用し、配列の先頭のポインタに保存することができます。反対に違うところは配列の先頭をポインタで上書きできないことです。配列の要素は変更可能ですが、配列の先頭アドレスは変更できません。

今回は次のものを追加していきます。

- 固定サイズの配列宣言、初期化リストはなし
- 1つの式内の右辺値としての配列インデックス
- 1つの代入式での左辺地としての配列インデックス

それぞれの配列で2次元以上のものは実装しません。

## 式にある括弧()

ある時点で`*(ptr+2)`を試してみたいです。これは`ptr[2]`と同じ結果となります。しかし今はまだ式で括弧が使えませんので、追加する必要があります。

### BNFでのC文法

1985年にJeff Leeによって書かれた、[BNF Grammar for C](https://www.lysator.liu.se/c/ANSI-C-grammar-y.html)というページがあります。これを参考にしあまり間違いを犯さないように気をつけたいです。

1点気をつけなければならないのは、Cでの二項演算子の優先順位を実装するのではなく、構文が再帰的な定義を利用して暗黙的な優先順位を実現していることです。

```c
additive_expression
        : multiplicative_expression
        | additive_expression '+' multiplicative_expression
        | additive_expression '-' multiplicative_expression
        ;
```

"additive_expression"をパースする一方で"multiplicative_expression"まで降りていくため、'*'や'/'オペレータに'+'や'-'よりも1段階上の優先順位を与えています。

式の優先順位の階層のトップはつぎのようになっています。

```c
primary_expression
        : IDENTIFIER
        | CONSTANT
        | STRING_LITERAL
        | '(' expression ')'
        ;
```

T_INTLITやT_IDENTトークンを見つけるために呼ぶ`primary()`はすでにあり、これはJeff LeeのC言語の文法に準拠しています。ですから、式内の括弧のパースを追加するには最適な場所です。

T_LPARENとT_RPARENはトークンとしてありますので、意味解析でやることはありません。

代わりに`primary()`を修正してパースを追加します。

```c
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  ...

  switch (Token.token) {
  case T_INTLIT:
  ...
  case T_IDENT:
  ...
  case T_LPAREN:
    // 括弧で囲われた式の始まり、'('をスキップする。(Beginning of a parenthesised expression, skip the '('.
    // 式と括弧の終わりをスキャンする。
    scan(&Token);
    n = binexpr(0);
    rparen();
    return (n);

    default:
    fatald("主要な式を想定していましたが、トークンが渡されました。", Token.token);
  }

  // 次のトークンをスキャンして葉ノードを返す。
  scan(&Token);
  return (n);
}
```

以上です。式に括弧を使えるように数行追加しただけです。追加したコードではswitch文を抜け出すのではなく、明示的に`rparen()`を呼び出してreturnしていることに気がついたかもしれません。コードにswitch文が残っている場合、最後のreturnの前にある`scan(&Token)`は最初の'('にマッチする')'が必要であることを厳密に強制しないでしょう。

`tests/input19.c`は括弧が動作するかテストしています。

```c
  a= 2; b= 4; c= 3; d= 2;
  e= (a+b) * (c+d);
  printint(e);
```

これは`6*5`であり、30が出力されるはずです。

## シンボルテーブルの変更

シンボルテーブルには（1つの値だけを持つ）スカラ変数と関数があります。これに配列を追加します。今後`sizeof()`オペレータを使って各配列の要素数を取得したくなるでしょう。`defs.h`の変更は次のようになります。

```c
// 構造上の型
enum {
  S_VARIABLE, S_FUNCTION, S_ARRAY
};

// シンボルテーブル構造体
struct symtable {
  char *name;                   // シンボル名
  int type;                     // シンボルの基本型
  int stype;                    // シンボルの構造上の型
  int endlabel;                 // S_FUNCTION用エンドラベル
  int size;                     // シンボルの要素数
};
```

とりあえず配列はポインタとして処理し、配列の型は配列の要素が`int`であれば"intへのポインタ"のように、"何らかのポインタ"となります。`sym.c`のaddglob()に引数を追加する必要もあります。

```c
int addglob(char *name, int type, int stype, int endlabel, int size) {
  ...
}
```

## 配列宣言のパース

一旦サイズをつけた配列の宣言だけをできるようにします。変数宣言のBNF構文は次のようになります。

```c
 variable_declaration: type identifier ';'
        | type identifier '[' P_INTLIT ']' ';'
        ;
```

`decl.c`の`var_declaration()`で次のトークンがなにか確認し、スカラ変数宣言と配列宣言を処理する必要があります。

```c
// スカラ変数かサイズを指定した配列の宣言をパースする。
// すでに識別子はスキャンされ型情報も持っている。
void var_declaration(int type) {
  int id;

  // グローバル変数Textには識別子名が入っている。
  // 次のトークンが '[' だった場合
  if (Token.token == T_LBRACKET) {
    // '['をスキップ
    scan(&Token);

    // 配列サイズがあるか調べる。
    if (Token.token == T_INTLIT) {
      // これを既知の配列とし、アセンブリにスペースを生成する。
      // 配列をその要素の型を指すポインタとして処理する。
      id = addglob(Text, pointer_to(type), S_ARRAY, 0, Token.intvalue);
      genglobsym(id);
    }

    // ']' がついているか確認
    scan(&Token);
    match(T_RBRACKET, "]");
  } else {
    ...
  }

    // 後続のセミコロンを取得
  semi();
}
```

かなり単純なコードだと思います。後ほど配列宣言に初期化指定子を追加します。

## 配列の保存先を生成

配列のサイズがわかるようになったので`cgglobsym()`をアセンブリでそのサイズを確保するよう修正できます。

```c
void cgglobsym(int id) {
  int typesize;
  // 型のサイズを取得
  typesize = cgprimsize(Gsym[id].type);

  // グローバル識別子とそのラベルを生成
  fprintf(Outfile, "\t.data\n" "\t.globl\t%s\n", Gsym[id].name);
  fprintf(Outfile, "%s:", Gsym[id].name);

  // スペースを生成
  for (int i=0; i < Gsym[id].size; i++) {
    switch(typesize) {
      case 1: fprintf(Outfile, "\t.byte\t0\n"); break;
      case 4: fprintf(Outfile, "\t.long\t0\n"); break;
      case 8: fprintf(Outfile, "\t.quad\t0\n"); break;
      default: fatald("cgglobsym 型のサイズが不明です: ", typesize);
    }
  }
}
```

このスペースによって次のような配列宣言が可能です。

```c
  char a[10];
  int  b[25];
  long c[100];
```

## 配列インデックスのパース

ここではあまり冒険はしたくありません。左辺値と右辺値として動作する基本的な配列インデックスに止めようと思います。`tests/input20.c`は実現したい機能が入っています。

```c
int a;
int b[25];

int main() {
  b[3]= 12; a= b[3];
  printint(a); return(0);
}
```

CのBNF文法に戻ると、括弧よりも若干優先度が低い配列インデックスが確認できます。

```c
primary_expression
        : IDENTIFIER
        | CONSTANT
        | STRING_LITERAL
        | '(' expression ')'
        ;

postfix_expression
        : primary_expression
        | postfix_expression '[' expression ']'
          ...
```

ですが今は配列インデックスも`primary()`関数でパースするつもりです。意味解析のコードは結局新しい関数を必要とするほど大きくなりました。

```c
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  int id;

  switch (Token.token) {
  case T_IDENT:
    // 変数、配列、関数呼び出しになり得る。
    // 判定するため次のトークンをスキャンする。
    scan(&Token);

    // '(' なら関数呼び出し
    if (Token.token == T_LPAREN) return (funccall());

    // '[' なら配列参照
    if (Token.token == T_LBRACKET) return (array_access());
```

以下は`array_access`関数です。

```c
// インデックスを配列へパースし、
// そのASTツリーを返す。
static struct ASTnode *array_access(void) {
  struct ASTnode *left, *right;
  int id;

  // 識別子が配列として定義されているか確認し、
  // その先頭を指す葉ノードを返す。
  if ((id = findglob(Text)) == -1 || Gsym[id].stype != S_ARRAY) {
    fatals("宣言されていない配列", Text);
  }
  left = mkastleaf(A_ADDR, Gsym[id].type, id);

  // '[' を取得
  scan(&Token);

  // 後続の式をパース
  right = binexpr(0);

  // ']' を取得
  match(T_RBRACKET, "]");

  // int型であるか確認
  if (!inttype(right->type))
    fatal("配列インデックスが整数型ではありません");

  // インデックスを要素の型のサイズでスケール
  right = modify_type(right, left->type, A_ADD);

  // 加算するオフセットを配列の先頭にオフセットが付加されたASTツリーを返し、
  // 要素を間接参照する。この段階ではまだ左辺値。
  // at this point.
  left = mkastnode(A_ADD, Gsym[id].type, left, NULL, right, 0);
  left = mkastunary(A_DEREF, value_at(left->type), left, 0);
  return (left);
}
```

配列`int x[20]`と配列インデックス`x[6]`の場合、インデックス(6)を`int`のサイズ(4)でスケールし、さらに配列の先頭のアドレスに加算する必要があります。それからこの要素は間接参照されます。次のことをできるようにしたいので左辺値としてマークしたままにしておきます。

```c
  x[6] = 100;
```

右辺値となる場合、`binexpre()`はA_DEREF ASTノードで`rvalue`フラグをセットすることになるでしょう。

### 生成されたASTツリー

`tests/input20.c`に戻り、配列インデックスを持つASTツリーを生成するコードは次のとおりです。

```c
  b[3]= 12; a= b[3];
```

`comp1 -T tests/input20.c`を実行すると次の結果が得られます。

```c
    A_INTLIT 12
  A_WIDEN
      A_ADDR b
        A_INTLIT 3    # 3 is scaled by 4
      A_SCALE 4
    A_ADD             # and then added to b's address
  A_DEREF             # and derefenced. Note, stll an lvalue
A_ASSIGN

      A_ADDR b
        A_INTLIT 3    # As above
      A_SCALE 4
    A_ADD
  A_DEREF rval        # but the dereferenced address will be an rvalue
  A_IDENT a
A_ASSIGN
```

### パースへの変更点他

`expr.c`のパーサにいくつか小さな変更点があり、デバッグに時間がかかりました。オペレータ優先順位を調べる関数への入力をより制限する必要がありました。

```c
// 二項演算子であるか確認してその優先順位を返す。
static int op_precedence(int tokentype) {
  int prec;
  if (tokentype >= T_VOID)
    fatald("op_precedence:優先順が無いトークンです", tokentype);
  ...
}
```

これ以外の変更点は式を配列インデックスとして使えるようになった(`x[a+1]`)ので、']'トークンが式を終わらせると備える必要があります。そのため`binexpr()`の終わりは次のようになります。

```c
    // 現在のトークンの詳細を更新する。
    // セミコロン、')' 、 ']' のいずれかであれば左のノードを返す。
    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN
        || tokentype == T_RBRACKET) {
      left->rvalue = 1;
      return (left);
    }
  }
```

## コード生成への変更

ありません。すでにコンパイラに必要なコンポーネントはすべて用意できました。整数値のスケーリング、変数のアドレスの取得などです。テストコードは次のようになります。

```c
  b[3]= 12; a= b[3];
```

これに対するx86-64アセンブリは次のようになります。

```assembly
        movq    $12, %r8
        leaq    b(%rip), %r9    # bのアドレスを取得
        movq    $3, %r10
        salq    $2, %r10        # 3を2回シフト i.e. 3 * 4
        addq    %r9, %r10       # bのアドレスに加算
        movq    %r8, (%r10)     # 12をb[3]に保存

        leaq    b(%rip), %r8    # bのアドレスを取得
        movq    $3, %r9
        salq    $2, %r9         # 3を2回シフト i.e. 3 * 4
        addq    %r8, %r9        # bのアドレスに加算
        movq    (%r9), %r9      # b[3]を%r9に読み込む
        movl    %r9d, a(%rip)   # aに保存
```

## まとめ

(構文の扱いという点において)基本的な配列宣言と配列式に必要なパースの変更はかなり簡単なことでした。難しかったことはASTツリーノードを正しくスケールさせ、先頭アドレスに加え、左辺値右辺値をセットすることでした。これがうまく行ったら既存のコード生成は正しいアセンブリ出力を生みました。

次回は文字と文字列リテラルを加え、それらを出力する方法を模索します。
