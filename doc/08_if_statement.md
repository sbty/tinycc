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

## if構文のパース

再帰下降パースであるため、if文のパースはそれほど悪くありません。

```c
// else句の有無に関わらずif文をパースする
// ASTを返す
struct ASTnode *if_statement(void) {
  struct ASTnode *condAST, *trueAST, *falseAST = NULL;

  // 'if('があるか確認
  match(T_IF, "if");
  lparen();

  // 続く式と')'をパース
  // ツリーの操作が比較であるか確認
  condAST = binexpr(0);

  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("無効な比較演算子");
  rparen();

  // 合成ステートメントのASTを取得
  trueAST = compound_statement();

  // 'else'があるならばスキップして合成ステートメントのASTを取得
  if (Token.token == T_ELSE) {
    scan(&Token);
    falseAST = compound_statement();
  }
  // 見ているステートメントに対してのASTを作成して返す
  return (mkastnode(A_IF, condAST, trueAST, falseAST, 0));
}
```

今は`if(x-2)`のような入力は扱いたくないので、`binexpr()`の二項演算子が6つの比較演算子A_EQ、A_NE、A_LT、A_GT、A_LE、A_GEのいずれかをルートとなるよう制限しました。

## 3番めの子

if_statement()の最後の行でASTノードを作っています。

```c
mkastnode(A_IF, condAST, trueAST, falseAST, 0);
```

ASTサブツリーが3つも有ります。if文は3つの子を持つことになります。

- 条件式を評価するサブツリー
- 直後につながる合成ステートメント
- elseキーワードに続くオプションの合成ステートメント

ですから3つの子を持つASTノード構造体が必要となります。(defs.h)

```c
// ASTノード型
enum {
  ...
  A_GLUE, A_IF
};

// 抽象構文ツリー(AST)
struct ASTnode {
  int op;                       // ツリーに対して実行される"操作"
  struct ASTnode *left;         // それぞれ左、真ん中、右ツリー
  struct ASTnode *mid;
  struct ASTnode *right;
  union {
    int intvalue;               // 整数値であるA_INTLIT用
    int id;                     // シンボルスロット番号であるA_IDENT用
  } v;
};
```

A_IFツリーは次のようになります。

```c
                     IF
                    / |  \
                   /  |   \
                  /   |    \
                 /    |     \
                /     |      \
               /      |       \
      condition   statements   statements
```

## ASTノードの接合

A_GLUE ASTノードタイプが追加されています。多くのステートメントからなる1つのASTツリーを作るので、一つにつなげる方法が必要です。

`compound_statement()`にあるループを見ていきましょう。

```c
if (left != NULL)
  left = mkastnode(A_GLUE, left, NULL, tree, 0);
```

サブツリーが増えるごとに既存のツリーに接合します。ですから次のようなステートメントは

```c
  stmt1;
  stmt2;
  stmt3;
  stmt4;
```

次のようになります。

```c
            A_GLUE
              /  \
          A_GLUE stmt4
            /  \
        A_GLUE stmt3
          /  \
      stmt1  stmt2
```

そしてこれは深さ優先で左から右へと走査していくと、正しい並びでアセンブリコードが生成されます。

## 汎用コードジェネレータ

ASTノードは複数の子を持つようになったので汎用コードジェネレータはやや複雑になりそうです。比較演算子についても、if文の中で行われている比較(分岐の反対へジャンプする)か、あるいは普通の式で行われているか(比較でレジスタを1か0にセットする)区別できる必要が有ります。

このため、親ASTノードの操作を渡せるよう、genAST()を修正しました。

```c
// AST、(あれば)右辺値を保持するレジスタ、親のAST操作を引数に取る
// 再帰的にアセンブリコードを生成する
int genAST(struct ASTnode *n, int reg, int parentASTop) {
   ...
}
```

### 特定のASTノードの処理

`genAST()`で特定のASTノードを処理しなければなりません。

```c
  switch (n->op) {
    case A_IF:
      return (genIFAST(n));
    case A_GLUE:
      // それぞれの子ステートメントを実行しレジスタを開放する
      genAST(n->left, NOREG, n->op);
      genfreeregs();
      genAST(n->right, NOREG, n->op);
      genfreeregs();
      return (NOREG);
  }
```

リターンしない場合、通常の二項演算子ASTノードの処理を続けます。ただし例外が有ります。比較ノードです。

```c
case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      // 親ASTノードがA_IFであれば後ろにジャンプがついた比較分を生成する
      // そうでないならレジスタを比較し、結果に基づいて0か1をセットする
      if (parentASTop == A_IF)
        return (cgcompare_and_jump(n->op, leftreg, rightreg, reg));
      else
        return (cgcompare_and_set(n->op, leftreg, rightreg));
```

次に新しく追加した関数`cgcompare_and_jump()`と`cgcompare_and_set()`を見ていきます。

### ifアセンブリコードの生成

A_IF ASTノードは専用の関数と、新規のラベル番号を生成する関数によって処理されます。

```c
// 新規ラベル番号を生成して返す
static int label(void) {
  static int id = 1;
  return (id++);
}

// if文のコードとオプションのelse句を生成する
static int genIFAST(struct ASTnode *n) {
  int Lfalse, Lend;

  // 2つのラベルを生成する
  // 1つはfalse合成ステートメント、
  // もう1つはif文全体の終了を示す
  // else句があるときはLfalse _is_ が終了を示すラベル

  Lfalse = label();
  if (n->right)
    Lend = label();

  // 条件式を生成、後ろにfalseラベルへのゼロジャンプが続く
  // レジスタとしてLfalseラベルを送ることで手抜きをする。
  genAST(n->left, Lfalse, n->op);
  genfreeregs();

  // true合成ステートメントを生成
  genAST(n->mid, NOREG, n->op);
  genfreeregs();

  // オプションのelse句があれば
  // 終わりまでスキップするジャンプを生成
  if (n->right)
    cgjump(Lend);

  // ここからfalseラベル
  cglabel(Lfalse);

  // オプションのelse句
  // false合成ステートメントとエンドラベルを生成
  if (n->right) {
    genAST(n->right, NOREG, n->op);
    genfreeregs();
    cglabel(Lend);
  }

  return (NOREG);
}
```

コードは実質次のことをしています。

```c
  genAST(n->left, Lfalse, n->op);       // 条件判定とLfalseへジャンプ
  genAST(n->mid, NOREG, n->op);         // 'if'の後ろの文
  cgjump(Lend);                         // Lendへジャンプ
  cglabel(Lfalse);                      // Lfalseラベル:
  genAST(n->right, NOREG, n->op);       // 'else'に続く文
  cglabel(Lend);                        // Lendラベル:
```

## x86-64コード生成関数
