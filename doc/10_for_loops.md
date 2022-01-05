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
