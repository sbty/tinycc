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

`expr.c`の互換品である`expr2.c`にprattパースを実装します。

はじめにそれぞれのトークンの優先度レベルを決定するコードが必要です。

```c
// 各トークンに対するオペレータ優先順位
static int OpPrec[] = { 0, 10, 10, 20, 20,    0 };
//                     EOF  +   -   *   /  INTLIT

// ２項演算子があることを確認して
// その優先度を返す
static int op_precedence(int tokentype) {
  int prec = OpPrec[tokentype];
  if (prec == 0) {
    fprintf(stderr, "構文エラー line %d, token %d\n", Line, tokentype);
    exit(1);
  }
  return (prec);
}
```

数値が高いほど優先されます。

OpPrec[]というテーブルがあるのにさらに関数を用意する理由は、構文エラーを見つけるためです。

234 101 + 12 という入力を考えてみます。最初の2トークンはスキャンできます。ですが2番目の101の優先順位を得るためだけに`OpPrec[]`を使った場合、オペレータがないことがわかりません。ですから`op_precedence()`関数で適切な文法構文となるよう矯正します。

これで各優先度レベルに関数を用意することなく、オペレータ優先順位のテーブルを利用した、1つの式関数でまかなえました。

```c
// ルートが二項演算子であるASTツリーを返す
// ptpは１つ前のトークンの優先順位
struct ASTnode *binexpr(int ptp) {
  struct ASTnode *left, *right;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンを取得
  left = primary();

  // トークンがなければ左ノードの値を返す
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // 1つ前のトークンより今のトークンの優先順位が高い限りループ
  while (op_precedence(tokentype) > ptp) {
    // 次の整数リテラルを取得
    scan(&Token);

    // サブツリーを構築するために今のトークンの優先順位で
    // binexpr()を再帰的に呼び出す
    right = binexpr(OpPrec[tokentype]);

    // サブツリーを結合する。同時にトークンをAST操作に変換する
    left = mkastnode(arithop(tokentype), left, right, 0);

    // 現在のトークンの詳細を更新
    // トークンがなければ左ノードを返す
    tokentype = Token.token;
    if (tokentype == T_EOF)
      return (left);
  }

  // 優先順位が同じか低いのであればツリーを返す
  return (left);
}
```

パーサの作りはこれまでと同様に再帰的です。今回は、呼び出される前に見つかったトークンの優先順位レベルを受け取ります。`main()`関数からは最低の優先順位、`0`で呼ばれますが、より高い(優先)値で自身を呼びだします。

コード自体は`multiplicative_expr()`と似ています。整数リテラルとして読み込み、オペレータのトークン型を取得し、ループによってツリーを構築していきます。

違いはループ条件と処理の対象だけです。

```c
multiplicative_expr():
  while ((tokentype == T_STAR) || (tokentype == T_SLASH)) {
    scan(&Token); right = primary();

    left = mkastnode(arithop(tokentype), left, right, 0);

    tokentype = Token.token;
    if (tokentype == T_EOF) return (left);
  }

binexpr():
  while (op_precedence(tokentype) > ptp) {
    scan(&Token); right = binexpr(OpPrec[tokentype]);

    left = mkastnode(arithop(tokentype), left, right, 0);

    tokentype = Token.token;
    if (tokentype == T_EOF) return (left);
  }
```

prattパーサでは現在のトークンよりも次のオペレータの優先度が高ければ、`primary()`で次の整数リテラルを取得するのではなく、`binexpr(OpPrec[tokentype])`と自身を呼び出し、オペレータの優先順位を引き上げます。

同じか低い優先順位のトークンがきたら単純に

```c
  return (left);
```

とします。これは大量のノードと呼び出し時点より高い優先度のオペレータを含んだサブツリーとなるか、同じ優先度のオペレータに対して1つだけの整数リテラルとなるでしょう。

これで式をパースするための関数が1つになりました。小さなヘルパー関数を使ってオペレータの優先順位を矯正し、それによりこの言語の意味付け(semantics)を実装しています。

## 2つのパーサを動かす

それぞれにパーサからプログラムを作成します。

```bash
$ make parser                                        # Pratt Parser
cc -o parser -g expr.c interp.c main.c scan.c tree.c

$ make parser2                                       # Precedence Climbing
cc -o parser2 -g expr2.c interp.c main.c scan.c tree.c

```

これまでの入力ファイルを使って2つのパーサをテストします。

```bash
$ make test
(./parser input01; \
 ./parser input02; \
 ./parser input03; \
 ./parser input04; \
 ./parser input05)
15                                       # input01
29                                       # input02
構文エラー line 1, token 5          # input03
認識できない文字です line 3       # input04
認識できない文字です line 1       # input05

$ make test2
(./parser2 input01; \
 ./parser2 input02; \
 ./parser2 input03; \
 ./parser2 input04; \
 ./parser2 input05)
15                                       # input01
29                                       # input02
構文エラー on line 1, token 5          # input03
認識できない文字です line 3       # input04
認識できない文字です line 1       # input05
```

## まとめ

これまでで以下のものができました。

- 言語のトークンを認識し返す、スキャナー
- 文法を認識し、構文エラーの通知と抽象構文木を構築するパーサ
- 言語の意味を実装するパーサのための、優先順位テーブル
- 深さ優先で抽象構文木を操作し、入力式の結果を計算するインタプリタ

コンパイラがまだありません。

次はインタプリタを置き換えます。算術オペレータを持つそれぞれのASTノードからx86-64アセンブリコードを生成する変換処理を記述していきます。また、ジェネレータが出力するアセンブリコードをサポートする、アセンブリの前文と後文も生成します。
