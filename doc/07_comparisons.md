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

パースとオペレータ優先順位については以上です。

## コード生成

追加された6つのオペレータは二項演算子であるため、`gen.c`の汎用コードジェネレータを対応させるのは容易です。

```c
case A_EQ:
    return (cgequal(leftreg, rightreg));
  case A_NE:
    return (cgnotequal(leftreg, rightreg));
  case A_LT:
    return (cglessthan(leftreg, rightreg));
  case A_GT:
    return (cggreaterthan(leftreg, rightreg));
  case A_LE:
    return (cglessequal(leftreg, rightreg));
  case A_GE:
    return (cggreaterequal(leftreg, rightreg));
```

## x86-64コードジェネレーション

少しトリッキーなことをします。Cでは比較演算子は値を返します。trueとみなせば1、falseとみなせば0となります。これを反映させるx86-64アセンブリコードが必要です。

幸いそのためのx86-64命令が有ります。ですがそのままでは少々問題が有ります。次のx86-64命令を考えてみます。

```assembly
  cmpq %r8,%r9
```

上の`cmpq`命令は%r9-%r8を実行しマイナスやゼロフラグを含む複数のステータスフラグをセットします。したがってフラグの組み合わせを見れば比較結果が確認できます。

| 比較     | 操作    | true時のフラグ |
| -------- | ------- | -------------- |
| %r8==%r9 | %r9-%r8 | 0              |
| %r8!=%r9 | %r9-%r8 | 非0            |
| %r8>%r9  | %r9-%r8 | 非0、負        |
| %r8<%r9  | %r9-%r8 | 非0、非負      |
| %r8>=%r9 | %r9-%r8 | 0または負      |
| %r8<=%r9 | %r9-%r8 | 0または非負    |

2つのフラグ値に基づいてレジスタに1または0をセットするx86-64命令は6つあり、`sete`、`setne`、`setg`、`setl`、`setge`、`setle`です。上の表の縦の並びの通りです。

問題はこれらの命令がレジスタの最下位バイトにしか値をセットしないことです。レジスタが最下位バイト以外のビットに値を持っていた場合、そのまま値は残ります。ですから十進数で1000の値を持つ変数に1をセットしようとすると1001となり望ましくない形になります。

解決案は`andq`を使ってsetX命令のあとにレジスタの不要なビットを取り除くことです。`cg.c`に汎用比較関数を用意します。

```c
// 2つのレジスタの比較
static int cgcompare(int r1, int r2, char *how) {
  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\t%s\n", how, breglist[r2]);
  fprintf(Outfile, "\tandq\t$255,%s\n", reglist[r2]);
  free_register(r1);
  return (r2);
}
```

`how`に`setX`命令が入っています。

```c
cmpq reglist[r2], reglist[r1]
```

も忘れないようにします。実態は`reglist[r1] - reglist[r2]`であり、本当にやりたいことだからです。

## x86-64レジスタ

少し脱線して、x86-64アーキテクチャのレジスタについて説明します。x86-64は複数の64ビット汎用レジスタを持ちます。ですが別のレジスタ名でアクセスすることが可能でレジスタを小分けに利用することができます。

| 64ビットレジスタ | 下位32ビット | 下位16ビット | 下位8ビット |
| ---------------- | ------------ | ------------ | ----------- |
| rax              | eax          | ax           | al          |
| rbx              | ebx          | bx           | bl          |
| rcx              | ecx          | cx           | cl          |
| rdx              | edx          | dx           | dl          |
| rsi              | esx          | sx           | sil         |
| rdi              | edi          | di           | dil         |
| rbp              | ebp          | bp           | bpl         |
| rsp              | esp          | sp           | spl         |
| r8               | r8d          | r8w          | r8b         |
| r9               | r9d          | r9w          | r9b         |
| r10              | r10d         | r10w         | r10b        |
| r11              | r11d         | r11w         | r11b        |
| r12              | r12d         | r12w         | r12b        |
| r13              | r13d         | r13w         | r13b        |
| r14              | r14d         | r14w         | r14b        |
| r15              | r15d         | r15w         | r15b        |

上記の表は、64ビットr8レジスタが"r8d"レジスタを使うことで下位32ビットにアクセスできることを示しています。同様にr8レジスタの下位16ビットが"r8w"、下位8ビットが"r8b"となっています。

`cgcompare()`関数内でコードは2つの64ビットレジスタを比較するために`reglist[]`配列を使っていますが、その後`breglist[]`関数にある名前を使って第二レジスタの8ビットバージョンフラグをセットしています。x86-64アーキテクチャでは`setX`命令は8ビットレジスタ名に対してのみ操作でき、そのため`breglist[]`配列が必要になります。

## 比較命令の作成

汎用関数が用意できなので6つの比較関数を記述していきます。

```c
int cgequal(int r1, int r2) { return(cgcompare(r1, r2, "sete")); }
int cgnotequal(int r1, int r2) { return(cgcompare(r1, r2, "setne")); }
int cglessthan(int r1, int r2) { return(cgcompare(r1, r2, "setl")); }
int cggreaterthan(int r1, int r2) { return(cgcompare(r1, r2, "setg")); }
int cglessequal(int r1, int r2) { return(cgcompare(r1, r2, "setle")); }
int cggreaterequal(int r1, int r2) { return(cgcompare(r1, r2, "setge")); }
```

他の二項演算子と同様に、片方のレジスタを開放しもう一方のレジスタに結果を入れて返します。

## 実践

input04ファイルを見ていきます。

```c
int x;
x= 7 < 9;  print x;
x= 7 <= 9; print x;
x= 7 != 9; print x;
x= 7 == 7; print x;
x= 7 >= 7; print x;
x= 7 <= 7; print x;
x= 9 > 7;  print x;
x= 9 >= 7; print x;
x= 9 != 7; print x;
```

これらの比較はすべて真であるため、9つの1が出力されるはずです。`make test`で確認しましょう。

最初の比較のアセンブリコードを見ていきます。

```assembly
        movq    $7, %r8
        movq    $9, %r9
        cmpq    %r9, %r8        # %r8-%r9 つまり 7-9 を実行
        setl    %r9b            # 7が9より小さければ%r9bに1をセット
        andq    $255,%r9        # %r9のほか全ビットを削除
        movq    %r9, x(%rip)    # xに結果を保存
        movq    x(%rip), %r8
        movq    %r8, %rdi
        call    printint        # xを出力
```

上記には非効率なアセンブリコードが残っています。最適化を考えるには早すぎます。

## まとめ

今回は簡単でしたが見返りは大きいものとなりました。次回はもっと複雑になります。

次回はif文を追加して、追加したばかりの比較演算子を使えるようにします。
