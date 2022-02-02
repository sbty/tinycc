# パート15: ポインタ、パート1

ポインタを追加していきます。特に以下のものを加えたいです。

- ポインタ変数の宣言
- ポインタへのアドレスの代入
- ポインタのDereferencingでポインタが指す値を取得

作業中であることを考慮し、とりあえず動く簡易バージョンを実装しますが、後により一般的にするため変更したり拡張する必要があるでしょう。

## 追加するキーワードとトークン

今回は2つのトークンを追加し、キーワードは追加しません。

- '&' T_AMPER
- '&&' T_LOGAND

今はまだT_LOGANDは不要ですが、`scan()`に以下のコードを追加するつもりです。

```c
    case '&':
      if ((c = next()) == '&') {
        t->token = T_LOGAND;
      } else {
        putback(c);
        t->token = T_AMPER;
      }
      break;
```

## 型のための追加コード

基本型をいくつか追加しました(defs.h)。

```c
// 基本型
enum {
  P_NONE, P_VOID, P_CHAR, P_INT, P_LONG,
  P_VOIDPTR, P_CHARPTR, P_INTPTR, P_LONGPTR
};
```

単項プレフィックスオペレータが増えました。

- 識別子のアドレスを取得する'&'
- ポインタをdereference(間接参照？)し、指す値を取得するための'*'

各オペレータが生成する式の型は、それぞれ動作する型によって異なります。`types.c`に少し関数を追加して、型を変えさせる必要が有ります。

```c
// 基本型を引数に、その型へのポインタを返す
int pointer_to(int type) {
  int newtype;
  switch (type) {
    case P_VOID: newtype = P_VOIDPTR; break;
    case P_CHAR: newtype = P_CHARPTR; break;
    case P_INT:  newtype = P_INTPTR;  break;
    case P_LONG: newtype = P_LONGPTR; break;
    default:
      fatald("pointer_to: 認識できない型です", type);
  }
  return (newtype);
}

// 基本型へのポインタを引数に、そのポインタが示す型を返す。
int value_at(int type) {
  int newtype;
  switch (type) {
    case P_VOIDPTR: newtype = P_VOID; break;
    case P_CHARPTR: newtype = P_CHAR; break;
    case P_INTPTR:  newtype = P_INT;  break;
    case P_LONGPTR: newtype = P_LONG; break;
    default:
      fatald("value_at: 認識できない型です。", type);
  }
  return (newtype);
}
```

これらの関数を使う場所を考えます。

## ポインタ変数の宣言

スカラ変数とポインタ変数を宣言できるようにしたいです。

```c
char a; char *b;
int  d; int  *e;
```

`decl.c`にはすでに型キーワードを型に変換する`parse_type()`関数があります。これを拡張して続くトークンをスキャンし、次のトークンが'*'だった場合に型を変換します。

```c
// 現在のトークンをパースして基本型のenum値を返す。
// 次のトークンをスキャンする。
int parse_type(void) {
  int type;
  switch (Token.token) {
    case T_VOID: type = P_VOID; break;
    case T_CHAR: type = P_CHAR; break;
    case T_INT:  type = P_INT;  break;
    case T_LONG: type = P_LONG; break;
    default:
      fatald("トークンの型が不正です。", Token.token);
  }

  // 1つ以上先の'*'トークンをスキャンし、今見ているトークンのポインタ型を決める。
  while (1) {
    scan(&Token);
    if (Token.token != T_STAR) break;
    type = pointer_to(type);
  }

  // スキャンした次のトークンはそのままにしておく。
  return (type);
}

これは将来的に次のことが可能になります。

```c
char *****fred;
```

これは`pointer_to()`がP_CHARPTRをP_CHARPTRPTRに(いまはまだ)変換できないので失敗します。ですが`parse_type`のコードはすでに対応済みです。

`var_declaration()`のコードはかなりうまくポインタ変数の宣言をパースしています。

```c
// 変数宣言のパース
void var_declaration(void) {
  int id, type;

  // 変数の型を取得する。
  // これもident()でスキャンする。
  type = parse_type();
  ident();
  ...
}
```

### プレフィクスオペレータ '*'と'&'

一旦宣言から離れて、式よりも前にくる'*'と'&'のパースを見てみましょう。BNF構文は次のようになります。

```c
 prefix_expression: primary
     | '*' prefix_expression
     | '&' prefix_expression
     ;
```

技術上は次の記述は許容されます。

```c
   x= ***y;
   a= &&&b;
```

2つのオペレータの無理な利用を防ぐため、意味チェックを追加しました。以下がコードです。

```c
// プレフィクス式をパースし、それを表すサブツリーを返す
struct ASTnode *prefix(void) {
  struct ASTnode *tree;
  switch (Token.token) {
    case T_AMPER:
      // 次のトークンを取得しプレフィクス式として再帰的にパースする
      scan(&Token);
      tree = prefix();

      // 識別子であるか確認
      if (tree->op != A_IDENT)
        fatal("& オペレータの後ろに識別子がありません");

      // オペレータをA_ADDRに、型を元の型を指すポインタに変換します。
      tree->op = A_ADDR; tree->type = pointer_to(tree->type);
      break;
    case T_STAR:
      // 次のトークンを取得しプレフィクスの式として再帰的にパースする
      scan(&Token); tree = prefix();

      // とりあえず間接演算子か識別子か確認する
      if (tree->op != A_IDENT && tree->op != A_DEREF)
        fatal("* オペレータの後ろに識別子も*もありません。");

      // A_DEREF操作をツリーの先頭につける
      tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0);
      break;
    default:
      tree = primary();
  }
  return (tree);
}
```

相変わらず再帰降下をやっていますが、入力ミスを防ぐためエラーチェックもしています。現時点では`value_at()`の制限により'*'オペレータを続けて並べることはできませんが、今後`value_at()`を変更したときに見直したりprefix()を変更する必要はないでしょう。

'prefix()'は'*'か'&'がないときに'primary()'を呼び出しています。これにより既存の'binexpr()'のコードを変更することができます。

```c
struct ASTnode *binexpr(int ptp) {
  struct ASTnode *left, *right;
  int lefttype, righttype;
  int tokentype;

  // 左のツリーを取得。
  // 同時に次のトークンを取得する。
  // 以前はprimary()を呼び出していた。
  left = prefix();
  ...
}
```
