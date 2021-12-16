# 本物のコンパイラ

今回はこれまで作ってきたインタラプタを、x86-64アセンブラコードを生成するものに置き換えていきます。

## インタプリタの修正

インタラプタは次のようになっていました。

```c
int interpretAST(struct ASTnode *n) {
  int leftval, rightval;

  if (n->left) leftval = interpretAST(n->left);
  if (n->right) rightval = interpretAST(n->right);

  switch (n->op) {
    case A_ADD:      return (leftval + rightval);
    case A_SUBTRACT: return (leftval - rightval);
    case A_MULTIPLY: return (leftval * rightval);
    case A_DIVIDE:   return (leftval / rightval);
    case A_INTLIT:   return (n->intvalue);

    default:
      fprintf(stderr, "不明なAST操作です %d\n", n->op);
      exit(1);
  }
}
```

`interpretAST()関数`は与えられたASTツリーを深さ優先で見ていきます。とにかく左のサブツリーを評価し、それから右のサブツリーへ向かいます。最後に現在のツリーの根本にある操作値(op value)を使ってこれらの子ツリーを最初の操作を操作します。

操作値が４つの算術演算子のいづれかであれば、実行されます。操作値が整数リテラルを示しているのであれば、リテラル値が返ります。

関数はこのツリーの最終的な値を返します。そして再帰であるため、複数のサブツリーから成る、全体のツリーの最終値を計算します。
