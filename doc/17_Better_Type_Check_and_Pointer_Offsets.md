# パート17: 型チェックの改良とポインタオフセット

少し前のパートで、ポインタを実装し型の互換性をチェックするコードを実装しました。その時は次のようなコードに

```c
  int   c;
  int  *e;

  e= &c + 1;
```

`&c`のポインタに1を加算するには`c`のサイズに変換され、メモリ上`c`の後ろにある次の`int`まで確実にスキップする必要があります。つまり整数型をスケールする必要が有ります。

これをポインタでも行う必要が有り、後で配列にも必要となります。次のコードを考えます。

```c
  int list[10];
  int x= list[3];
```

これを行うには`list[]`の初期アドレスを見つけ、それからインデックス位置3にある要素を見つけるために`int`のサイズを3回加算する必要が有ります。

このときは2つの型に互換性があるか判断する`type_compatible()`という関数を`type.c`に用意し、大きな整数型と同じサイズにするために小さな型を拡張する必要がある場合は明記するようにしていました。ただしこの拡張は別の場所で行っていました。実際コンパイラでは3箇所で処理することで実現しています。

## `type_compatible()`の置き換え

`type_compatible()`が示すのであれば、ツリーを大きな整数型にマッチするようA_WIDENするでしょう。現在はツリーを値が型のサイズでスケールされるようA_SCALEする必要が有ります。拡張コードの重複をリファクタリングしたいです。

そのために`type_compatible()`を捨てて置き換えました。かなり考えることがあり、おそらく再度改善や拡張が必要となるでしょう。設計を見ていきます。

既存の`type_compatible()`:

- 2つの型の値を引数にとり、オプションで向きもある
- 型に互換性があればtrueを返す
- 一方を拡張する必要があればA_WIDENを返す
- ツリーにはA_WIDENノードを繋げない
- 型に互換性がなければfalseを返す
- ポインタ型を扱わない

型の比較の利用例を見ていきます。:

- 2つの式に二項演算子を適用する場合、型に互換性はあるのか、拡張、拡縮は必要か
- `print`文を実行する場合、式は整数で拡張は必要か
- 代入文を実行する場合、式を拡張する必要はあるか、左辺値の型に一致しているか
- `return`文を実行する場合、式を拡張する必要はあるか、関数の戻り値の型に一致しているか

この例で2つの式があるのは1つだけです。したがって1つのASTツリーと変換先の型を引数に取る関数を新しく用意しました。二項演算子の例ではこの関数を2回呼び出し、それぞれでなにが起こるか確認します。

## `modify_type`の導入

`types.c`の`modify_type`は`type_compatible()`のための置き換えコードです。関数のAPIは次のようになっています。

```c
// ASTツリーと変換先の型を引数に取り、この型と互換性を保てるよう、
// 拡張や拡縮によってツリーを修正する。
// 修正が発生しなければ元のツリーを、修正したのであれば修正後のツリーを、
// または引数の型と互換性がなければNULLを返す。
// これが二項演算子の一部となるのであればAST操作(AST op)は0以外となる。
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op);
```

疑問: なぜツリーに対して何らかの二項演算子が実行されなければならないのでしょうか？ 答えはポインタからは加算か減算しかできないからです。それ以外はできません。例えば以下のコードです。

```c
  int x;
  int *ptr;

  x= *ptr;    // OK
  x= *(ptr + 2);   // ptrが指している場所からint2つ分先
  x= *(ptr * 4);   // 意味がない
  x= *(ptr / 13);  // 意味がない
```

とりあえず次のようなコードになっています。多くの具体的なテストがあり、今の所考えられるテストを合理化する方法が思い浮かびません。これも後で拡張する必要が出るでしょう。

```c
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op) {
  int ltype;
  int lsize, rsize;

  ltype = tree->type;

  // int型の比較
  if (inttype(ltype) && inttype(rtype)) {

    // 型が同じなら何もしない
    if (ltype == rtype) return (tree);

    // それぞれの型のサイズを取得
    lsize = genprimsize(ltype);
    rsize = genprimsize(rtype);

    // ツリーサイズが大きすぎないかチェック
    if (lsize > rsize) return (NULL);

    // 右側へ拡張
    if (rsize > lsize) return (mkastunary(A_WIDEN, rtype, tree, 0));
  }

  // 左側にあるポインタの処理
  if (ptrtype(ltype)) {
    // 右側と同じ型であり、二項演算子でなければOK
    if (op == 0 && ltype == rtype) return (tree);
  }

  // A_ADD または A_SUBTRACT 操作に対してのみ拡縮可能
  if (op == A_ADD || op == A_SUBTRACT) {

    // 左がint、右がポインタ型で元の型のサイズが
    // 1より大きければ左をスケールする
    if (inttype(ltype) && ptrtype(rtype)) {
      rsize = genprimsize(value_at(rtype));
      if (rsize > 1)
        return (mkastunary(A_SCALE, rtype, tree, rsize));
    }
  }

  // ここに到達したら型は非互換
  return (NULL);
}
```

AST A_WIDENとA_SCALE操作に加算するコードはここ1箇所だけで行っています。A_WIDEN操作は子の型を親の型へ変換します。A_SCALE操作は(`defs.h`の)ASTノードのユニオンフィールドに追加したサイズ分、子の値を乗算します。

```c
// Abstract Syntax Tree structure
struct ASTnode {
  ...
  union {
    int size;                   // A_SCALEのためのスケールするサイズ
  } v;
};
```

## 追加した`modify_type()` APIの利用

追加したAPIにより`stmt.c`と`expr.c`にあったA_WIDENへの重複したコードを削除できるようになりました。ですがこの新しい関数は1つのツリーだけを引数に取ります。本当に1つしかツリーがないのであればこれで問題ありません。今の所`stmt.c`の`modify_type()`の呼び出しは3回あります。すべて似ているので`assignment_statement()`を見ていきます。

```c
  // 左辺値の代入のためのASTノードを作成
  right = mkastleaf(A_LVIDENT, Gsym[id].type, id);

  ...
  // 後続の式をパース
  left = binexpr(0);

  // 2つの型に互換性があるか確認
  left = modify_type(left, right->type, 0);
  if (left == NULL) fatal("代入の式に互換性がありません");
```

これまでのコードよりもずっとスッキリしています。
