# オペレータの優先順位

前回作成したところまででは、乗算よりも加算が優先されるなどオペレータの優先順位がおかしかったのでこれを修正していきます。そのためには次の２つの方法があります。

- 言語の文法でオペレータの優先順位を明示する
- オペレータの優先順位テーブルを使い既存のパーサに影響を与える

## オペレータの優先順位の明示

前回までの構文は以下のものです。

```c
expression: number
          | expression '*' expression
          | expression '/' expression
          | expression '+' expression
          | expression '-' expression
          ;

number:  T_INTLIT
         ;
```

4つの算術オペレーターに何も違いはありません。これを区別するよう文法を修正します。

```c
expression: additive_expression
    ;

additive_expression:
      multiplicative_expression
    | additive_expression '+' multiplicative_expression
    | additive_expression '-' multiplicative_expression
    ;

multiplicative_expression:
      number
    | number '*' multiplicative_expression
    | number '/' multiplicative_expression
    ;

number:  T_INTLIT
         ;
```

加算式と乗算式の2つのタイプを用意します。整数値が必ず乗算式の1部になっています。この強制により`*`と`/`オペレータはどちらかの数字とより密接に結び付けられ、高い優先順位を持つことになります。

加算式はそれ自体が乗算式であるか、加算(つまり乗算)式に`+`や`-`が続くいて別の乗算式がきます。加算式は乗算式よりも低い優先順位となりました。

## 上記の処理を再帰降下パーサで実行

expr2.cで実装していきます。

`*`と`/`を処理する`multiplicative_expr()`関数と`+`と`-`を処理する`additive_expr()`関数を用意します。

どちらの関数もオペレータ＋何かを読み取ります。それから同じ優先順位のオペレータが続く限り各関数は入力に対してさらにパースを行い最初の演算子で左右の半分を結合します。

ですが`additive_expr()`は優先度の高い`multiplicative_expr()`に従わなければなりません。次のようになります。

```c:additive_expr()
// ルートが２項演算子であるASTツリーを返す。
struct ASTnode *additive_expr(void) {
  struct ASTnode *left, *right;
  int tokentype;

  // 現在より優先度の高い左のサブツリーを取得
  left = multiplicative_expr();

  // トークンが残ってなければ左ノードだけを返す。
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // 同一優先度のトークンを繰り返し処理する
  while (1) {
    // 次の整数リテラルを取得
    scan(&Token);

    // 現在より優先度の高い右側のサブツリーを取得
    right = multiplicative_expr();

    // 優先度の低いオペレータで2つのサブツリーを結合
    left = mkastnode(arithop(tokentype), left, right, 0);

    // 同じ優先度の次のトークンを取得
    tokentype = Token.token;
    if (tokentype == T_EOF)
      break;
  }

  // 出来上がったツリーを返す
  return (left);
}
```

まず最初のオペレータが優先度の高い`*`か`/`だった場合、`multiplicative_expr()`を呼び出します。この関数は`+`か`-`といった優先度の低いオペレータを見つけたときは抜けます。

したがってwhileループに到達したのであれば1つの`+`か`-`オペレータがあることがわかります。トークンがなくなる、つまりT_EOFが出るまでループします。

左右のサブツリーが用意できたらループの最後に出たオペレータで結合します。これを繰り返してASTツリーを作ります。

```c
       +
      / \
     +   6
    / \
   2   4
```

ですが`multiplicative_expr()`がより高いオペレータを見つけた場合、複数のノードを持つサブツリーをそれらで結合することになるでしょう。

## multiplicative_expr()

```c
// ルートが２項演算子'*' か '/'であるASTツリーを返す
struct ASTnode *multiplicative_expr(void) {
  struct ASTnode *left, *right;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンを取得
  left = primary();

  // トークンが残ってなければ左ノードだけを返す
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // トークンが'*' か '/'であるかぎりループ
  while ((tokentype == T_STAR) || (tokentype == T_SLASH)) {
    // 次の整数リテラルを取得
    scan(&Token);
    right = primary();

    // 左の整数リテラルとを結合
    left = mkastnode(arithop(tokentype), left, right, 0);

    // 現在のトークンの詳細を更新
    // トークンが残ってなければ左のノードを返す
    tokentype = Token.token;
    if (tokentype == T_EOF)
      break;
  }

  // 作成したツリーを返す
  return (left);
}
```

整数値を取得するために`primary()`を呼ぶ以外は`additive_expr()`と似ています。ループするのは`*`や`/`といった優先度の高いオペレータがあるときだけです。優先度の低いオペレータが見つかり次第、その時点まで構築したサブツリーを返します。優先度の低いオペレータを処理するため、値は`additive_expr()`に返ります。

## 上記の問題点

上記の、オペレータ優先順位の明示による再帰降下パーサを構築することは、すべての関数呼び出しが正しい優先順位になる必要があるため、非効率となります。またそれぞれのオペレータ優先順位レベルを処理するための関数が必要となるので、大量のコードとなります。

## 代替案: Pratt Parsing

コード量を削減する方法の１つにPratt Parsingがあります。これは構文内の明示的な優先順位を置き換える関数を持つのではなく、各トークンと紐付いた優先順位を表す値のテーブルを用意します。

[Pratt Parsers: Expression Parsing Made Easy ](https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/)



## expr.c: Pratt Parsing
