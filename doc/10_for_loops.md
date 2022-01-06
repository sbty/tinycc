# 10 forループ

forループを追加していきます。実装する上で解決すべきジレンマがあるので、どうやって解決したかよりも先に説明します。

## forループ構文

for文について詳しいと想定します。例を挙げます。

```c
for(i=0; i<MAX;i++)
  printf("%d\n",i);
```

BNF構文で表現します。

```c
for_statement: 'for' '(' preop_statement ';'
                          true_false_expression ';'
                          postop_statement ')' compound_statement  ;

 preop_statement:  statement  ;
 postop_statement: statement  ;

```

`preop_statement`はループ開始よりも先に実行されます。その後、(if文はおけないなど)どういったアクションを取りうるのか正確に制限する必要が有ります。それから`true_false_expression`が評価され、真であればループは合成ステートメントを実行します。この処理を終えると`postop_statement`が実行されループは先頭に戻り、再度`true_false_expression`を行います。

## ジレンマ

`postop_statement`は`compound_statement`よりも先にパースされますが、`postop_statement`のコードの生成は`compound_statement`の後に行わなければならないというジレンマが有ります。

この問題の解決策はいくつか有ります。1つは`compound_statement`のアセンブリコードを一時的なバッファにいれ、`postop_statement`のコードが生成された後にバッファを再送することです。

このプロジェクトではASTツリーを採用しています。これを利用して正しい順番でアセンブリコードの生成をします。

## ASTツリー

for文は4つの構造上の部品から成ります。

1. preop_statement
2. true_fasle_expression
3. postop_statement
4. compound_statement

ASTノード構造体を変えたくありません。forループはwhileループを拡張したものとして視覚化することが可能です。

```c
preop_statement;
   while ( true_false_expression ) {
     compound_statement;
     postop_statement;
   }
```

既存のノードタイプからこの構造を反映したASTツリーを作れることは可能です。

```c
         A_GLUE
        /     \
   preop     A_WHILE
             /    \
        decision  A_GLUE
                  /    \
            compound  postop
```

whileループを抜けるときに`compound_statement`と`postop_statement`をスキップするよう、2つを接着する必要が有りました。

つまりT_FORトークンが必要になるのですが、ASTノードタイプはいりません。ですからコンパイラへの変更はスキャンとパースです。

## トークンとスキャン

キーワード'for'とそれに紐付いたトークン、T_FORを追加します。それ以外に大きな変更はありません。

## ステートメントのパース

パーサに構造上の変更を加える必要が有ります。FOR文法は`preop_statement`と`postop_statement`の単一のステートメントにしたいです。今の所`compound_statement()`関数は対応する'}'を見つけるまでループします。これを`compound_statement()`が`single_statement()`を呼び出して1つのステートメントを取得するよう、切り離す必要が有ります。

ですがもう1つジレンマが有ります。`assignment_statement()`にある代入ステートメントを見てみましょう。パーサがステートメントの終わりにあるセミコロンを見つける必要が有ります。

compound_statementにとってはこれでいいですが、FORループではうまくいきません。次のようにしようと思います。

```c
for (i=1; i<10; i=i+1;)
```

それぞれの代入ステートメントはセミコロンで終わる必要があるからです。

必要なのは、単一のステートメントパーサにセミコロンをスキャンさせず、合成ステートメントに任せることです。ほかステートメント(代入ステートメントに挟まれているものなど)にセミコロンをスキャンさせ、連続するif文に挟まれていないステートメントにはスキャンさせません。

以上の説明で、新しい単一ステートメントと合成ステートメントのパースコードを見ていきましょう。

```c
// 単一ステートメントをパースしてASTを返す
static struct ASTnode *single_statement(void) {
  switch (Token.token) {
    case T_PRINT:
      return (print_statement());
    case T_INT:
      var_declaration();
      return (NULL);  // ここではASTを生成しない
    case T_IDENT:
      return (assignment_statement());
    case T_IF:
      return (if_statement());
    case T_WHILE:
      return (while_statement());
    case T_FOR:
      return (for_statement());
    default:
      fatald("構文エラー token", Token.token);
  }
}

// 合成ステートメントをパースしそのASTを返す
struct ASTnode *compound_statement(void) {
  struct ASTnode *left = NULL;
  struct ASTnode *tree;

  // '{'があるか確認
  lbrace();

  while (1) {
    // 単一ステートメントのパース
    tree = single_statement();

    // 後ろにセミコロンが必要なステートメントのチェック
    if (tree != NULL &&
  (tree->op == A_PRINT || tree->op == A_ASSIGN))
      semi();

    // 新規のツリーそれぞれに対し、左が空であれば左に保存、
    // あるいは左と新規ツリーを結合する。
    if (tree != NULL) {
      if (left == NULL)
  left = tree;
      else
  left = mkastnode(A_GLUE, left, NULL, tree, 0);
    }
    // '}'を見つけたらそこまでスキップしてASTを返す
    if (Token.token == T_RBRACE) {
      rbrace();
      return (left);
    }
  }
}
```

`print_statement()`と`assignment_statement()`での`semi()`の呼び出しを削除しています。

## FORループのパース

上記のFORループ向けのBNF構文があれば、パースは単純です。必要なASTツリーの概要を考慮すると、このツリーを作り上げるコードもまた単純です。以下がコードになります。

```c
// FORステートメントをパースしてそのASTを返す
static struct ASTnode *for_statement(void) {
  struct ASTnode *condAST, *bodyAST;
  struct ASTnode *preopAST, *postopAST;
  struct ASTnode *tree;

  // 'for' '('があるか確認
  match(T_FOR, "for");
  lparen();

  // preop statementと';'があるか確認
  preopAST= single_statement();
  semi();

  // 条件式と';'を取得
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("不正な比較演算子です");
  semi();

  // postop statementと')'を取得
  postopAST= single_statement();
  rparen();

  // for文の中身である合成ステートメントを取得
  bodyAST = compound_statement();

  // 現段階では4つのサブツリーはNULLであってはならない
  // 後ほど一部が省略されている場合の意味解析を変更する予定

  // 合成ステートメントとpostopツリーを結合
  tree= mkastnode(A_GLUE, bodyAST, NULL, postopAST, 0);

  // 条件式と新しい本文でwhlieループを作成
  tree= mkastnode(A_WHILE, condAST, NULL, tree, 0);

  // preopツリーとA_WHILEツリーを結合
  return(mkastnode(A_GLUE, preopAST, NULL, tree, 0));
}
```

##
