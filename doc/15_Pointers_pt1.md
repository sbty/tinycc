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
