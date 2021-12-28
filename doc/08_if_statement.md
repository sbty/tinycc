# 07 if文

値が比較できるようになったので、if文を追加します。はじめにif文の汎用的な構文と、それらがどのようにアセンブリに変換されるか見ていきましょう。

## if文のつくり

if文の構文は次のようになっています。

```c
if (条件が真であれば)
  最初のブロックを実行
else
  別のブロックを実行
```

次に、これがどのようにアセンブリに変換されるかです。ひっくり返して比較をし、その比較が真であればジャンプ/枝分かれをすることがわかりました。

```assembly
  ひっくり返した比較を実行
  真であればL1へジャンプ
  最初のブロックのコードを実行
  L2へジャンプ
L1:
  別のブロックを実行
L2:

```

`L1`と`L2`はアセンブリのラベルです。

## 自作コンパイラによるアセンブリの生成

今の所、比較に基づいてレジスタをセットするコードを出力していますが

```c
  int x; x = 7 < 9;
```

これは次のようになります。

```assembly
  movq    $7, %r8
  movq    $9, %r9
  cmpq    %r9, %r8
  setl    %r9b        ;値が小さければセット
  andq    $255,%r9
```

if文であれば比較の反対側にジャンプする必要が有ります。

```c
if(7 < 9)
```

これは次のようになります。

```assembly
    movq    $7, %r8
    movq    $9, %r9
    cmpq    %r9, %r8
    jge     L1         ;等しいかより大きければジャンプ
    ....
L1:
```

if文の実装は以上です。途中の変更点や追加された部分を取り上げていきます。

## 新たなトークンとDangling_else

我々の言語にたくさんの新しいトークンが必要となりそうです。[Dangling_else](https://en.wikipedia.org/wiki/Dangling_else)も回避したいです。そのため、すべてのステートメントが`{`...`}`の中括弧で囲まれるよう文法を変更しました。このグルーピングを合成ステートメントと名付けます。if文を作るために`(`...`)`括弧に加えて`if`と`else`キーワードも必要です。`defs.h`にこれらを追加します。

```c
T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN,
  // キーワード
  ..., T_IF, T_ELSE
```

## トークンのスキャン

1文字トークンは明確であるはずなので、それらをスキャンするコードは書きません。キーワードもかなり明確ですが、`scan.c`の`keyword()`にスキャン用コードを用意します。

```c
switch (*s) {
    case 'e':
      if (!strcmp(s, "else"))
        return (T_ELSE);
      break;
    case 'i':
      if (!strcmp(s, "if"))
        return (T_IF);
      if (!strcmp(s, "int"))
        return (T_INT);
      break;
    case 'p':
      if (!strcmp(s, "print"))
        return (T_PRINT);
      break;
  }
```

## 追加するBNF文法

文法が肥大化し始めたので手直しを加えました。

```c
compound_statement: '{' '}'          // 空、empty
      |      '{' statement '}'
      |      '{' statement statements '}'
      ;

 statement: print_statement
      |     declaration
      |     assignment_statement
      |     if_statement
      ;

 print_statement: 'print' expression ';'  ;

 declaration: 'int' identifier ';'  ;

 assignment_statement: identifier '=' expression ';'   ;

 if_statement: if_head
      |        if_head 'else' compound_statement
      ;

 if_head: 'if' '(' true_false_expression ')' compound_statement  ;

 identifier: T_IDENT ;
```

`true/false式`の定義は省きましたが、もう少し演算子が増えたら追加することになるでしょう。

`if文`の構文は(else句のない)`if_head`、または`if_head`に`else`と`compound_statement(合成ステートメント)`が続くもののいずれかであることに注意してください。

つまり先頭の`compound_statement`は`{...}`で囲われ、`else`キーワードの後ろに来る`compound_statement`も同様に囲われます。入れ子となったif文があれば、次のようになるはずです。

```c
if (条件1がtrueであれば) {
    if (条件２がtrueであれば) {
      statements;
    } else {
      statements;
    }
  } else {
    statements;
  }
```

これで`if`や`else`がどこに属するか、曖昧さがなくなります。これはdangling else問題を解決します。後ほど{'...'}' を省略できるようにします。

## 合成ステートメントのパース

既存の`void statements()`関数は`compound_statement()`として次のようになりました。

```c
// 合成ステートメントのパース
// ASTを返す
struct ASTnode *compound_statement(void) {
  struct ASTnode *left = NULL;
  struct ASTnode *tree;

  // `{` を探す
  lbrace();

  while(1) {
    switch ( Token.token){
      case T_PRINT:
        tree = print_statement();
        break;
      case T_INT:
        var_declaration();
        tree = NULL;
        break;
      case T_IDENT:
        tree = assignment_statement();
        break;
      case T_IF:
        tree = if_statement();
        break;
      case T_RBRACE:
        // `}`を見つけたら
        // スキップしてASTを返す
        rbrace();
        return (left);
      default:
        fatald("構文エラー、token", Token.token);

    } // switch

    // 新しいツリーごとに左が空であれば左に保存、
    // または新規ツリーごと左につなげる
    if(tree){
      if(left == NULL)
        left = tree;
      else
        left = mkastnode(A_GLUE, left, NULL, tree, 0);
    }

  } // while
}
```

まず、コードはパーサに`lbrace()`で合成ステートメントが`{`で始まるよう強制している点に注意してください。`rbrace()`で対応する`}`を見つけたときだけ抜けることができます。

次に、`compound_statement()`でやっているように、`print_statement()`、`assignment_statement()`、`if_statement()`はすべてASTツリーを返している点に着目します。これまでのコードでは、`print_statement()`は`genAST()`、ついで`genprintint()`を呼び出して式を評価していました。同様に、`assignment_statement()`も`genAST()`を呼び出して代入を行っていました。

これで一方にはASTツリー、もう一方にもASTツリーがあることになります。一度だけASTツリーを生成し、一度だけ`genAST()`を呼び出してアセンブリコードを生成するのはある程度合理的に思えます。

これは必須ではありません。例えばSubCでは式に対してのみASTを生成します。言語の構造的な部分において、例えばステートメントなどは、SubCでは私が過去のバージョンでやっていたように、特定のコードジェネレータを呼び出しています。

パーサで入力全体に対して1つのASTツリーを生成することにしています。入力がパースされると、1つのASTツリーから出力アセンブリが生成されます。

後ほど各関数ごとにASTツリーを生成することになるでしょう。
