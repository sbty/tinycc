# パーサの導入

パーサの役割は入力の構造的要素と構文を理解し、ある言語の構文として受け入れられることを保証することです。

前回までですでに以下の要素を実装しました。

- 数学の基本演算子である `+` `-` `*` `/`
- `0` 〜 `9`からなる１桁以上の整数

## BNF Backus-Naur Form

```c:BNF
expression: number
          | expression '*' expression
          | expression '/' expression
          | expression '+' expression
          | expression '-' expression
          ;

number:  T_INTLIT
         ;

`|`で構文上のオプションは区切られるので

```c

- expressionは１つ以上の整数か、または
- expressionは`*`トークンで分割された2つの式か
- expressionは`/`トークンで分割された2つの式か
- expressionは`+`トークンで分割された2つの式か
- expressionは`-`トークンで分割された2つの式
- numberは常に単一のT_INTLITトークン(INT LITERAL)

```

次の疑似コードで考えます。

```c

function expression() {
  // スキャンして最初のトークンが数字であるか見る。数字でなければエラー。
  // 次のトークンを取得
  // 入力の最後に到達したらreturn

  // それ以外はexpression()を呼び出す。

}

```

上のコードに`2 + 3 - 5 T_EOF`を入力した場合次のようになります。

```c
expression(0)
// 2を見て、数値とする
// 次のトークンを見る、+でありT_EOFではないことがわかる
// expression()を呼ぶ

  expression(1)
    // 3を見て、数値とする。
    // 次のトークンを見る、-であるT_EOFではないことがわかる
    // expression()を呼ぶ

    expression(2)
      // 5を見て、数値とする。
      // 次のトークンを見る、-T_EOFなのでexpression(2)から値を返す。

  expression(1)から値が返る
expression(0)から値が返る

```

## 抽象構文木(Abstract Syntax Trees)

意味解析を行うには入力を解釈するか、あるいはアセンブリなどの別フォーマットに変換するコードが必要です。
このプロジェクトでは入力に対するインタプリタを作ります。そのために入力を抽象構文木、通称ASTへ変換します。
ASTの解説
<https://medium.com/basecs/leveling-up-ones-parsing-game-with-asts-d7a6fc2400ff>

```c
// AST ノードタイプ
enum {
  A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE, A_INTLIT
};

// Abstract Syntax Tree構造体
struct ASTnode {
  int op;                               // このツリーが実行する操作
  struct ASTnode *left;                 //
  struct ASTnode *right;
  int intvalue;                         // A_INTLITのときの整数
};
```

`op`が`A_ADD`や`A_SUB`であるASTノードは`left`と`right`が指す２つのASTノードを持ちます。

`op`がA_INTLITであるノードは整数です。これは子を持たず`intvalue`に値を持ちます。

## ASTノードとツリーの構築

`tree.c`はASTを構築するためのコードです。

```c
// 生成したASTノードを返す
struct ASTnode *mkastnode(int op, struct ASTnode *left,
                          struct ASTnode *right, int intvalue) {
  struct ASTnode *n;
こを
  // 新規にASTノードを確保する
  n = (struct ASTnode *) malloc(sizeof(struct ASTnode));
  if (n == NULL) {
    fprintf(stderr, "Unable to malloc in mkastnode()\n");
    exit(1);
  }
  // 引数をコピーする
  n->op = op;
  n->left = left;
  n->right = right;
  n->intvalue = intvalue;
  return (n);
}
```

これによりASTノードの葉(子がいないノード)や子を一つ持ったより詳細な関数を記述することが可能となります。

```c
// AST葉ノード
struct ASTnode *mkastleaf(int op, int intvalue) {
  return (mkastnode(op, NULL, NULL, intvalue));
}

// 単一の子を持つASTノード
struct ASTnode *mkastunary(int op, struct ASTnode *left, int intvalue) {
  return (mkastnode(op, left, NULL, intvalue));
```

## 繊細な式パーサ

トークンの値をASTノードの操作として再利用することもできますが、このプロジェクトではのトークンとASTノードは別の概念として扱います。トークン値をASTノードの操作値にマッピングする関数をexpr.cに記述します。

```c
// トークンをASTの操作へ変換
int arithop(int tok) {
  switch (tok) {
    case T_PLUS:
      return (A_ADD);
    case T_MINUS:
      return (A_SUBTRACT);
    case T_STAR:
      return (A_MULTIPLY);
    case T_SLASH:
      return (A_DIVIDE);
    default:
      fprintf(stderr, "arithop() に不明なトークンがあります line %d\n", Line);
      exit(1);
  }
}

```

default文は与えられたトークンがどのASTノード型にも変換できないときです。

次のトークンが整数リテラルか調べ、リテラルを保持するASTノードを作る関数が必要です。

```c
// 主要な要素をパースし、それらを表すASTノードを返す。
static struct ASTnode *primary(void) {
  struct ASTnode *n;

  // INTLITトークンであればAST葉ノードを作り次のトークンでスキャン
  // そうでなければ他のトークン型では構文エラー
  switch (Token.token) {
    case T_INTLIT:
      n = mkastleaf(A_INTLIT, Token.intvalue);
      scan(&Token);
      return (n);
    default:
      fprintf(stderr, "構文エラー line %d\n", Line);
      exit(1);
  }
}
```

Tokenは`data.h`で宣言されているグローバル変数であり、直前に入力からスキャンしたトークンが入っています。

これでパーサを記述する用意ができました。

```c
// ルートが2項演算子であるASTツリーを返す
struct ASTnode *binexpr(void) {
  struct ASTnode *n, *left, *right;
  int nodetype;

  // 左の整数リテラルを取得
  // 同時に次のトークンを取得
  left = primary();

  // トークンがなければleftのノードを返す
  if (Token.token == T_EOF)
    return (left);

  // トークンをノード型へ変換
  nodetype = arithop(Token.token);

  // 次のトークンを取得
  scan(&Token);

  // 再帰的にright型のツリーを取得
  right = binexpr();

  // サブツリーを持つツリーを作る
  n = mkastnode(nodetype, left, right, 0);
  return (n);
}
```

現時点ではオペレータの優先順位が等しいため`2 * 3 + 4 * 5`などの計算がうまく行きません。

## ツリーの解釈

現状のおかしなASTツリーを修正するため、次の疑似コードが必要です。

```c
interpretTree:
  // まずleft側のサブツリーを解釈し、その値を取得
  // 次にright側のサブツリーを解釈し、その値を取得
  // ２つのサブツリーにある値に対してツリーのルートのノードにある操作を実行
  // 値を返す
```
