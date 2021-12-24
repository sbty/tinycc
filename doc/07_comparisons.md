# 07 比較演算子

既存の演算子と同じく二項演算子なのでそれほど難しくないでしょう。

6つの比較演算子`==`、`!=`、`<`、`>`、`<=`、`>=`を追加するのに必要な変更を見ていきましょう。

## 新規トークンの追加

6つの新たなトークンが必要です。`defs.h`似追加します。

```c
// トークンタイプ
enum {
  T_EOF,
  T_PLUS, T_MINUS,
  T_STAR, T_SLASH,
  T_EQ, T_NE,
  T_LT, T_GT, T_LE, T_GE,
  T_INTLIT, T_SEMI, T_ASSIGN, T_IDENT,
  // キーワード
  T_PRINT, T_INT
}
```

これまではトークンは優先順位がありませんでしたが、優先度が低いものから高いものになるよう並び替えました。

## トークンのスキャン

次にスキャンです。`=`と`==`、`<`と`<=`、`>`と`>=`をそれぞれ区別する必要があります。入力から1文字多く読み取り、不要であれば差し戻さなければなりません。`scan.c`の`scan()`にコードを追加します。

```c
case '=':
    if ((c = next()) == '=') {
      t->token = T_EQ;
    } else {
      putback(c);
      t->token = T_ASSIGN;
    }
    break;
  case '!':
    if ((c = next()) == '=') {
      t->token = T_NE;
    } else {
      fatalc("解釈できない文字", c);
    }
    break;
  case '<':
    if ((c = next()) == '=') {
      t->token = T_LE;
    } else {
      putback(c);
      t->token = T_LT;
    }
    break;
  case '>':
    if ((c = next()) == '=') {
      t->token = T_GE;
    } else {
      putback(c);
      t->token = T_GT;
    }
    break;
```

新しいトークンT_EQと比較と代入で混乱しないよう、`=`トークンの名前をT_ASSIGNに変更しています。

## 追加された式コード

これで6つの新しいトークンをスキャンできるようになりました。次にこれらが式に出てきたらパースし、オペレータの優先順位を強制させなければなりません。

このプロジェクトはSubCを参考にセルフコンパイルを目指しているため、通常のCのオペレータ優先順位に従います。つまり比較演算子は乗算や除算よりも低い優先順位を持ちます。

トークンをASTノードタイプに対応させるために使っているswitch文が大きくなる一方です。そこでASTノードをすべての二項演算子と1:1で対応させるよう、ノードタイプを並び替えました(`defs.h`)。

```c
// ASTノードタイプ
// 行ごとに関連性有り
enum {
  A_ADD=1, A_SUBTRACT, A_MULTIPLY, A_DIVIDE,
  A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE,
  A_INTLIT,
  A_IDENT, A_LVIDENT, A_ASSIGN
};
```

`expr.c`でトークンからASTノードへの変換を簡略化し、トークンの優先順位を追加しました。

```c
// 二項演算子トークンをAST操作へ変換
// トークンからAST操作への1:1の対応に依存する
static int arithop(int tokentype) {
  if (tokentype > T_EOF && tokentype < T_INTLIT)
    return(tokentype);
  fatald("構文エラー, token", tokentype);
}

// 各トークンの優先順位
// defs.hのトークンの並びに一致していなければならない
static int OpPrec[] = {
  0, 10, 10,                    // T_EOF, T_PLUS, T_MINUS
  20, 20,                       // T_STAR, T_SLASH
  30, 30,                       // T_EQ, T_NE
  40, 40, 40, 40                // T_LT, T_GT, T_LE, T_GE
};
```

パースとオペレータ優先順位については異常です。
