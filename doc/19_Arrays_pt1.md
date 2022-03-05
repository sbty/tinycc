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
