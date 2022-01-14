# 12 型 パート1

今の所、かなり簡略化した関数宣言だけがあります。コンパイラへ型の追加をはじめようと思います。

## どの型から始めるか?

グローバル変数での`char`と`int`から始めます。関数にはすでに`void`キーワードを追加してあります。次は関数の返り値を追加します。いまのところ`void`はあるのですが、完全に処理してるわけではありません。

明らかに`char`は`int`よりもずっと制限された値です。SubCのように`char`の範囲を0〜255とし、、`int`は符号付き値の範囲にするつもりです。

つまり`char`の値を`int`に拡張することができますが、`int`の値を`char`の範囲に狭める際には、開発者に警告しなければなりません。

## 追加するキーワードとトークン

'char'キーワードとT_CHARトークンだけ追加します。

## 式の型

今後はすべての式に型が付きます。

- 整数リテラル、例えば56は`int`
- 計算式、例えば45-12は`int`
- 変数、`x`を`char`として宣言したのであれば、右辺値は`char`

式を必要に応じて拡張したり、必要であれば狭めるのを拒否するため、式を評価するのであればそれぞれの式の型を追跡する必要があるでしょう。

SubCコンパイラではNilは単一の左辺値構造体を生成しました。この構造体を指すポインタが再帰パーサに渡され、パースが指す地点の式の型を追跡しています。

ここでは違う手法を試みます。抽象構文ツリーを修正し、ある地点でのツリーの型を保持する、`type`フィールドを用意します。`defs.h`に追加した型は以下のとおりです。

```c
// 基本となる型
enum {
  P_NONE, P_VOID, P_CHAR, P_INT
};
```

いい名前が思いつかなかったのでSubCではNilsと呼んでいたものを、基本となる型(primitive types)としました。P_NONE値はASTノードが式を表さず、型がないことを示します。例としてA_GLUEノード型はステートメントをつなげるものですが、左側のステートメントが生成されると、型については言及がありません。

`tree.c`を見ると、ASTノードを構築する関数が、ASTノード構造体に追加されたtypeフィールドに代入するよう修正されていることがわかります。

```c
struct ASTnode {
  int op;                       // このツリーに対して実行される"操作"
  int type;                     // このツリーが生成する式の型
  ...
};
```

## 変数宣言とその型

現時点で少なくとも2つグローバル変数を宣言する方法が有ります。

```c
int x; int char;
```

これをパースする必要が有りますが、それぞれの変数の型をどのように記録すればいいでしょうか。`symtable`を変更する必要が有ります。今後必要になるシンボルの"構造上の型"の詳細も追加しました(`defs.h`)。

```c
// 構造上の型
enum {
  S_VARIABLE, S_FUNCTION
};

// シンボルテーブル構造体
struct symtable {
  char *name;     // シンボルの名前
  int type;       // シンボルの基本型
  int stype;      // シンボルの構造上の型
};
```

これらの追加フィールドを初期化するため`sym.c`の`newglob()`にコードを追加しました。

```c
int addglob(char *name, int type, int stype) {
  ...
  Gsym[y].type = type;
  Gsym[y].stype = stype;
  return (y);
}
```

## 変数宣言のパース

そろそろ変数のパースから型のパースを切り離す時期です。そこで`decl.c`を次のようにしました。

```c
// 今見ているトークンをパースし
// プリミティブ型のenum値を返す
int parse_type(int t) {
  if (t == T_CHAR) return (P_CHAR);
  if (t == T_INT)  return (P_INT);
  if (t == T_VOID) return (P_VOID);
  fatald("不正な型, token", t);
}

// 変数の宣言をパース
void var_declaration(void) {
  int id, type;

  // 変数の型を取得し、それから識別子を取得する
  type = parse_type(Token.token);
  scan(&Token);
  ident();
  id = addglob(Text, type, S_VARIABLE);
  genglobsym(id);
  semi();
}
```

## 式の型の取り扱い

これで以下のものが用意できました。

- 3つの型: char int void
- 変数宣言の型を見つけるパース
- シンボルテーブル内のそれぞれの変数の型の捕捉
- それぞれのASTノードの式の型の保持

構築したASTノードに型を埋めてやる必要が有ります。それから型を拡張するのか、あるいは型の衝突を避けるか判断します。

## プライマリターミナルのパース

整数リテラル値と変数識別子のパースに取り掛かります。1つ解決したい課題が有ります。

```c
char j; j = 2;
```

上記の2をP_INTと記録すると、P_CHARである変数`j`に保存しようとしたときに、値を狭めることができなくなります。とりあえずP_CHARとして小さな整数リテラル値を保持する意味解釈のコードを追加しました。

```c
// 主要素をパースしてそれを表すASTノードを返す
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  int id;

  switch (Token.token) {
    case T_INTLIT:
      // INTLITトークンであれば葉ASTノードを作る
      // P_CHAR範囲内であればP_CHARとして作る
      if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
        n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
      else
        n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
      break;

    case T_IDENT:
      // この識別子があるか調べる
      id = findglob(Text);
      if (id == -1)
        fatals("不明な変数", Text);

      // 葉ASTノードを作る
      n = mkastleaf(A_IDENT, Gsym[id].type, id);
      break;

    default:
      fatald("構文エラー, token", Token.token);
  }

  // 次のトークンをスキャンし葉ノードを返す
  scan(&Token);
  return (n);
}
```

識別子についてはブローバルシンボルテーブルから簡単に型の詳細を取得できます。
