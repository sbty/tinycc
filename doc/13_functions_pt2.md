# 13 関数 パート2

今回は関数呼び出しと戻り値を追加していきます。以下の内容です。

- 実装済みの関数の定義
- 1つだけの引数を伴う関数呼び出し、今はまだ使えない
- 関数からの戻り値
- ステートメントと式としての関数呼び出しの利用
- void関数が値を返さないことと、非void関数が値を返すことの確認

## キーワードとトークンの追加

これまでは`int`は8バイト(64ビット)としてきましたが、Gccは`int`を4バイト(32ビット)として扱っています。そこで`long`型を導入することにしました。

- `char`は1バイト幅
- `int`は4バイト(32bits)幅
- `long`は8バイト(64bits)幅

値を返す機能も必要なので`long`と`return`のキーワードを用意し、トークン'T_LONG'、'T_RETURN'と紐付けます。

## 関数呼び出しのパース

とりあえず関数呼び出しに使うBNF構文は以下のものです。

```c
function_call: identifier '(' expression ')'   ;
```

関数は名前の後ろに括弧のペアがきます。括弧の中に1つの引数が必要です。これを式と単体のステートメントの両方で使えるようにしたいです。

そこで`expr.c`にある関数呼び出しパーサ、`funccall()`から取り掛かります。呼ばれたときに、識別子はすでにスキャンされ関数名はグローバル変数`Text`にあります。

```c
// 単一の式を引数とする関数呼び出しをパースし
// ASTを返す
struct ASTnode *funccall(void) {
  struct ASTnode *tree;
  int id;

  // 識別子が定義されているか調べ
  // それから葉ノードをつくる
  if ((id = findglob(Text)) == -1) {
    fatals("宣言されていない関数", Text);
  }
  // '(' を取得
  lparen();

  // 後続の式をパース
  tree = binexpr(0);

  // 関数呼び出しASTノードを作成
  // 関数の戻り値をノードの型として保存
  // 関数のシンボルIDを記録
  tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);

  // ')' を取得
  rparen();
  return (tree);
}
```

関数や変数が定義されたとき、シンボルテーブルは構造上の型S_FUNCTIONとS_VARIABLEでそれぞれマークされます。本来はここに識別子が本当にS_FUNCTIONか確認するコードを加えるべきです。

単項ASTノード、A_FUNCCALLを追加します。子は引数として渡す単体の式です。ノード内にある関数のシンボルidを保存し、関数の戻り値の型を記録します。

## 不要なトークン

パースに関して問題が有ります。次の2つを区別しなければなりません。

```c
x = fred + jim;
x = fred(5) + jim;
```

1つ先のトークンを見て'('があるか確認する必要が有ります。あるならば関数呼び出しになります。でえすがこれを行うと既存のトークンを失います。これを解決するため、不要なトークンを差し戻せるようスキャナを変更します。トークンを返すとき、新しいものではなく次のトークンを返すようにしました。`scan.c`の追加コードは以下になります。

```c
// 拒否されたトークンへのポインタ
static struct token *Rejtoken = NULL;

// スキャンしたばかりのトークンを拒否
void reject_token(struct token *t) {
  if (Rejtoken != NULL)
    fatal("トークンを2回拒否はできません。");
  Rejtoken = t;
}

// スキャンして入力から見つけたトークンを返す
// トークンが有効であれば1，トークンがなければ0を返す
int scan(struct token *t) {
  int c, tokentype;

  // 拒否したトークンがあればそれを返す
  if (Rejtoken != NULL) {
    t = Rejtoken;
    Rejtoken = NULL;
    return (1);
  }

  // 通常のスキャンを続ける
  ...
}
```

## 式としての関数呼び出し

`expr.c`を見ていきます。`primary()`変数名と関数呼び出しを区別しなければなりません。追加コードは以下になります。

```c
// 主要素をパースしそれらを表すASTノードを返す
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  int id;

  switch (Token.token) {
    ...
    case T_IDENT:
      // 変数か関数呼び出しとなる
      // 判別するために次のトークンをスキャン
      scan(&Token);

      // '(' なら関数呼び出し
      if (Token.token == T_LPAREN)
        return (funccall());

      // 関数呼び出しでないならトークンを拒否
      reject_token(&Token);

      // 通常の変数パースを続ける
      ...
}
```
