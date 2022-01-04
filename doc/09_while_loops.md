# 09 whileループ

今回はwhileループを追加していきます。毎回ループの先頭へ戻ることを除けば、else句がないif文とwhile文は似ているところも有ります。

つまり以下の内容は

```c
while (条件が真か) {
    statements;
  }
```

次のように変換されます。

```c
Lstart: 条件を判定
  条件が偽であればLendへジャンプ
  ステートメント
  Lstartへジャンプ
Lend:
```

if文で使ったスキャン、パース、コード生成構造体を拝借し、いくらか変更を加えればwhile文も処理できそうです。

では見ていきましょう。

## 追加するトークン

'while'キーワードに対する新トークン、T_WHILEが必要になります。`scan.c`と`defs.h`へ変更を加えます。

## while構文のパース

whileループのBNF文法は

```c
// while_statement: 'while' '(' true_false_expression ')' compound_statement  ;
```

`stmt.c`にこれをパースする関数が必要です。if文のパースよりもずっとシンプルです。

```c
// while文のパース
// ASTを返す
struct ASTnode *while_statement(void) {
  struct ASTnode *condAST, *bodyAST;

  // 'while' '('があるか確認
  match(T_WHILE, "while");
  lparen();

  // 後ろに続く式と')'をパースし
  // ツリーの操作が比較であることを確認
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("不正な比較演算子");
  rparen();

  // 合成ステートメントのASTを取得
  bodyAST = compound_statement();

  // このステートメントのASTを作成し返す
  return (mkastnode(A_WHILE, condAST, NULL, bodyAST, 0));
}
```

新しいASTノード型、A_WHILEを`defs.h`に追加しました。このノードは条件式を判定するために左に子サブツリー、whileループの中身となる合成ステートメントを右の子サブツリーに持ちます。

## 汎用コード生成

はじめと終わりのラベルを作り、条件を判定しループを抜ける適切なジャンプを挿入し、ループの先頭へ戻る機能が必要です。ここもif文を生成するよりずっと簡単です。`gen.c`は次のようになります。

```c
// while文とオプションのelse句のコードを生成
static int genWHILE(struct ASTnode *n) {
  int Lstart, Lend;

  // はじめと終わりラベルを生成し
  // スタートラベルを出力
  Lstart = label();
  Lend = label();
  cglabel(Lstart);

  // 条件式とそれに続く、終わりラベルへのジャンプを生成
  // Lfalseラベルをレジスタとして送信するインチキをする
  genAST(n->left, Lend, n->op);
  genfreeregs();

  // while本文の合成ステートメントを生成
  genAST(n->right, NOREG, n->op);
  genfreeregs();

  // 最後に条件判定へ戻るコードと終わりラベルを出力する
  cgjump(Lstart);
  cglabel(Lend);
  return (NOREG);
}
```

比較演算子の親ASTノードの解釈がA_WHILEとなる可能性があったので、genAST()で比較演算子を次のようにする必要が有りました。

```c
case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      // 親ASTノードがA_IFかA_WHLEであれば、
      // 後ろにジャンプがつく比較分を生成する
      // そうでなければレジスタを比較して、結果に応じて
      // 1か0をセットする
      if (parentASTop == A_IF || parentASTop == A_WHILE)
        return (cgcompare_and_jump(n->op, leftreg, rightreg, reg));
      else
        return (cgcompare_and_set(n->op, leftreg, rightreg));
```

以上でwhileループの実装に必要なものが揃いました。

## 追加した文をテスト

入力ファイルを`tests/`ディレクトリへ移動しました。`make test`するとこのディレクトリへ行き、すべての入力ファイルをコンパイルし、正しいとされる出力と比較します。

```bash
cc -o comp1 -g cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c
      sym.c tree.c
(cd tests; chmod +x runtests; ./runtests)
input01: OK
input02: OK
input03: OK
input04: OK
input05: OK
input06: OK
```

`make test6`とすると`tests/input06`ファイルをコンパイルします。

```c
{ int i;
  i=1;
  while (i <= 10) {
    print i;
    i= i + 1;
  }
}
```

上記の内容は1から10の数字を出力します。

```bash
cc -o comp1 -g cg.c decl.c expr.c gen.c main.c misc.c scan.c
      stmt.c sym.c tree.c
./comp1 tests/input06
cc -o out out.s
./out
1
2
3
4
5
6
7
8
9
10
```

コンパイルによるアセンブリの出力は次のようになります。

```assembly
  .comm i,8,8
  movq  $1, %r8
  movq  %r8, i(%rip)  # i= 1
L1:
  movq  i(%rip), %r8
  movq  $10, %r9
  cmpq  %r9, %r8    # i <= 10?
  jg  L2            # 以上であればL2へジャンプ
  movq  i(%rip), %r8
  movq  %r8, %rdi   # iを出力
  call  printint
  movq  i(%rip), %r8
  movq  $1, %r9
  addq  %r8, %r9    # iに1を加算
  movq  %r9, i(%rip)
  jmp L1            # ループ先頭へ戻る
L2:
```

## まとめ

if文とwhile文は似ている箇所が多いため、すでにif文を実装したあとでは簡単に済みました。

チューリング完全になったと言えそうです。

- 無限のストレージ、つまり無限の変数
- if文のような、変数の値に応じた条件分岐を行う機能
- while文のような、進行方向を変更する機能

次回はforループについて実装していきます。
