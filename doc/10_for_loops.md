# 10 forループ

forループを追加していきます。実装する上で解決すべきしわ寄せがあるので、どうやって解決したかよりも先に説明します。

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

## しわ寄せ
