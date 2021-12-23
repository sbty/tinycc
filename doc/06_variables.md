# 06 変数

グローバル変数を実装していきます。

## 変数に求めるもの

次のものを実現したいです。

- 変数の宣言
- 変数に値を保存
- 変数に割り当て

`input02`は次のようになっています。

```c
int fred;
int jim;
fred= 5;
jim= 12;
print fred + jim;
```

明らかな違いは文法が式内に変数宣言、割り当てステートメント、変数名を持つようになったことです。まずは変数をどのように実装するか見ていきましょう。

## シンボルテーブル

すべてのコンパイラはシンボルテーブルを必要とします。後ほどグローバル変数以外のものを追加しますが、現時点ではテーブルのエントリ構造(`defs.h`)は次のようになっています。

```c
// シンボルテーブル構造
struct symtable {
  char *name; // シンボル名
}
```

`data.h`にシンボルの配列を用意します。

```c
#define NSYMBOLS        1024            // シンボルテーブルにあるエントリの数
extern_ struct symtable Gsym[NSYMBOLS]; // グローバルのシンボルテーブル
static int Globs = 0;                   // グローバルシンボルスロットの次の空いている位置
```

`Globs`は実際には`sym.c`にあり、このファイルがシンボルテーブルを管理します。現時点で次の管理用関数を用意します。

- `int findglob(char *s)`: シンボルsがグローバルシンボルテーブルにあるか見る。あればスロット位置、なければ-1を返す
- `static int newglob(void)`: 追加するグローバルシンボルスロットの位置を取得する。空きがなければ終了
- `int addglob(char *name)`: シンボルテーブルにグローバルシンボルを追加する。シンボルテーブルのスロット番号を返す

## スキャンと新規トークン

入力ファイルの例を見ると、いくつか新しくトークンが必要です。

- T_INTとして'int'
- T__EQUALSとして'='
- T_IDENTとして識別子名

'='のスキャンを`scan()`に追加するのは簡単です。

```c
case '=':
  t->token = T_EQUALS; break;
```

`keyword()`に'int'キーワードを追加します。

```c
case 'i':
    if (!strcmp(s, "int"))
      return (T_INT);
    break;
```

識別子についてはすでに`scanident()`を使って`Text`変数に単語を保存しています。単語がキーワードでないときに終了させるのではなく、T_IDENTトークンを返すようにします。

```c
if (isalpha(c) || '_' == c) {
      // キーワードか識別子として読み取る
      scanident(c, Text, TEXTLEN);

      // キーワードとして解釈されたらそのトークンを返す
      if (tokentype = keyword(Text)) {
        t->token = tokentype;
        break;
      }
      // キーワードでなければ識別子であるはず
      t->token = T_IDENT;
      break;
    }
```

## 新たな文法

入力する言語の文法は

```c
statements: statement
      |      statement statements
      ;

 statement: 'print' expression ';'
      |     'int'   identifier ';'
      |     identifier '=' expression ';'
      ;

 identifier: T_IDENT
      ;
```

識別子はT_IDENTトークンとして返され、printステートメントをパースするコードはすでにあります。ですが現時点で3種類のステートメントがあり、それぞれが1つ処理する関数を用意するのは理にかなっています。`stmt.c`の最上位の`statements()`関数は次のようになっています。

```c
// 1つ以上のステートメントをパース
void statements(void) {

  while (1) {
    switch (Token.token) {
    case T_PRINT:
      print_statement();
      break;
    case T_INT:
      var_declaration();
      break;
    case T_IDENT:
      assignment_statement();
      break;
    case T_EOF:
      return;
    default:
      fatald("構文エラー, token %d", Token.token);
    }
  }
}
```

これまでのprintステートメントは`print_statement()`へ移しました。

## 変数宣言

変数宣言を見ていきます。これは新たに追加した`decl.c`で、今後様々なタイプの宣言が追加されていくでしょう。

```c
// 変数宣言をパース
void var_declaration(void) {

  // 識別子とセミコロンの前にintトークンがあるか調べる
  // Textには識別牛の名前が入る
  // 既知の識別子として追加
  match(T_INT, "int");
  ident();
  addglob(Text);
  genglobsym(Text);
  semi();
}
```

`ident()`と`semi()`関数はmatch()関数のラッパー関数。

```c
void semi(void) { match(T_SEMI, ";"); }
void ident(void) { match(T_IDENT, "identifier"); }
```

## 割り当てステートメント

以下は`stmt.c`の`assignment_statement()`のコードです。

```c
void assignment_statement(void) {
  struct ASTnode *left, *right, *tree;
  int id;

  // 識別子が登録されているか調べる
  ident();

  // 定義されていれば葉ノードを作る
  if ((id = findglob(Text)) == -1) {
    fatals("宣言されていない変数", Text);
  }
  right = mkastleaf(A_LVIDENT, id);

  // イコール記号があるか調べる
  match(T_EQUALS, "=");

  // 続く式をパース
  left = binexpr(0);

  // 割り当てのASTツリーを作る
  tree = mkastnode(A_ASSIGN, left, right, 0);

  // 割り当てのアセンブリコードを生成
  genAST(tree, -1);
  genfreeregs();

  // 後ろにセミコロンがあるか調べる
  semi();
}
```

ASTノードに新しい種類が追加されています。A_ASSIGNは左の子として式を受け取り、右の子として代入を受け取ります。右の子はA_LVIDENTノードとなるでしょう。

A_LVIDENTと呼ぶのは、これがlvalue識別子を表しているからです。

lvalueは特定の場所と紐付いた値です。ここではそのメモリアドレスは変数の値を持っています。

```c
area = width * height;
```

右手の結果(つまりrvalue)を左手の変数(lvalue)に代入します。rvalueは特定の場所と結びついていません。式の結果はおそらく任意のレジスタに入るでしょう。

また代入ステートメントは構文を持ちます。

```c
identifier '=' expression ';'
```

A_ASSIGNノードの左側のサブツリーの式を作り、右側のA_LVIDENTの詳細を保存します。変数にこれらを保存するために式を評価する必要があるからです。

## AST構造への変更

A_INTLIT ASTノードにある整数リテラル値か、またはA_IDENT ASTノードのシンボルの詳細を保存する必要が出てきました。そのためdefs.hでAST構造体にunionを追加しました。

```c
// 抽象構文木構造体
struct ASTnode {
  int op;                       // このツリーに対して実行される"操作"
  struct ASTnode *left;         // 左右の子ツリー
  struct ASTnode *right;
  union {
    int intvalue;               // 整数値であるA_INTLIT
    int id;                     // シンボルのスロット番号であるA_IDENTnumber
  } v;
};
```

## 代入コードの生成

`gen.c`にある`genAST()`への変更は次のとおりです。

```c
int genAST(struct ASTnode *n, int reg) {
  int leftreg, rightreg;

  // 左右のサブツリーの値を取得
  if (n->left)
    leftreg = genAST(n->left, -1);
  if (n->right)
    rightreg = genAST(n->right, leftreg);

  switch (n->op) {
  ...
    case A_INTLIT:
    return (cgloadint(n->v.intvalue));
  case A_IDENT:
    return (cgloadglob(Gsym[n->v.id].name));
  case A_LVIDENT:
    return (cgstorglob(reg, Gsym[n->v.id].name));
  case A_ASSIGN:
    // やることを終えたので結果を返す
    return (rightreg);
  default:
    fatald("不明なAST操作", n->op);
  }
```

左手の子ASTを先に評価します。それから左手のサブツリーの値を持っているレジスタ番号を返します。それからレジスタ番号を右手のサブツリーへ渡します。A_LVIDENTに対してこうするのは、`cg.c`のcgstorglob()関数に代入式の結果のrvalueをどのレジスタが持っているか知らせるためです。

次のASTツリーを考えてみましょう。

```c
           A_ASSIGN
          /        \
     A_INTLIT   A_LVIDENT
        (3)        (5)
```

A_INTLIT操作を評価するため`leftreg = genAST(n->left, -1);`を呼び出します。これは`return (cgloadint(n->v.intvalue));`、つまり3をレジスタに読み込み、そのレジスタIDを返します。

それからA_LVIDENT操作を評価するため`rightreg = genAST(n->right, leftreg);`を呼び出します。これは`return (cgstorglob(reg, Gsym[n->v.id].name));`、つまりレジスタを`Gsym[5]`にある名前の変数に保存します。

そしてA_ASSIGNへと移ります。ここでの仕事は以上です。rvalueはまだレジスタに残っているのでそのままにしておきます。後ほど以下のような式を実現します。

```c
a = b = c = 0;
```

これは代入がただのステートメントではなく、式でもある場合です。

## x86-64コードの生成

`cgload()`関数名を`cgloadint()`に変更し、より具体的にしました。グローバル変数から値を読み込む関数も用意しました。(`cg.c`)

```c
int cgloadglob(char *identifier) {
  // 新たにレジスタを取得
  int r = alloc_register();

  // 初期化するコードを出力
  fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", identifier, reglist[r]);
  return (r);
}
```

同様にレジスタの中身を変数に保存する関数が必要です。

```c
// レジスタの値を変数に保存
int cgstorglob(int r, char *register) {
  fprintf(Outfile, "tmovqt%s, %s(\%%rip)\n", reglist[r], identifier);
  return (r);
}
```

新規に整数を入れるためのグローバル変数を作る関数も必要です。

```c
// グローバルシンボルを生成
void cgglobsym(char *sym){
  fprintf(Outfile, "\t.comm\t%s, 8, 8\n", sym);
}
```

パーサが直接このコードにアクセスできるようにはしないので、インターフェースとして振る舞う関数をgen.cの汎用コードジェネレータに用意します。

```c
void genglobsym(char *s) { cgglobsym(s); }
```

## 式の中にある変数

これで変数への代入が可能となりました。ですが変数の値のをどうやって式へ入れるのでしょうか。すでに整数リテラルを取得するための`primary()`はあります。修正して変数の値を読み込めるようにしていきます。

```c
// 主な要素をパースして、それらを表現するASTノードを返す
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  int id;

  switch (Token.token) {
  case T_INTLIT:
    // INTLITトークンであればAST葉ノードを作る
    n = mkastleaf(A_INTLIT, Token.intvalue);
    break;

  case T_IDENT:
    // 識別子があるか調べる
    id = findglob(Text);
    if (id == -1)
      fatals("不明な変数", Text);

    // AST葉ノードを作る
    n = mkastleaf(A_IDENT, id);
    break;

  default:
    fatald("構文エラー, token", Token.token);
  }

  // 次のトークンをスキャンして葉ノードを返す
  scan(&Token);
  return (n);
}
```

case文のT_IDENTの構文チェックは、変数を利用する前に、その変数が宣言されているか確認しています。

変数の値を受け取るAST葉ノードはA_IDENTノードです。変数へ保存する葉ノードはA_LVIDENTノードです。これは左辺値と右辺値の違いです。

## テスト

変数宣言に関してはこれくらいにして、`input02`ファイルで試してみましょう。

```c
int fred;
int jim;
fred= 5;
jim= 12;
print fred + jim;
```

テストを実行します。

```bash
$ make test
cc -o comp1 -g cg.c decl.c expr.c gen.c main.c misc.c scan.c
               stmt.c sym.c tree.c
...
./comp1 input02
cc -o out out.s
./out
17
```

5+12、あるいは17である`fred + jim`が計算できました。`out.s`の新たなアセンブリ行は以下の通りです。

```assembly
        .comm   fred,8,8                # Declare fred
        .comm   jim,8,8                 # Declare jim
        ...
        movq    $5, %r8
        movq    %r8, fred(%rip)         # fred = 5
        movq    $12, %r8
        movq    %r8, jim(%rip)          # jim = 12
        movq    fred(%rip), %r8
        movq    jim(%rip), %r9
        addq    %r8, %r9                # fred + jim
```

## その他の変更

`misc.c`に致命的なエラーを報告しやすくするヘルパー関数を追加しました。

```c
// 問題となるメッセージを出力
void fatal(char *s) {
  fprintf(stderr, "%s on line %d\n", s, Line); exit(1);
}

void fatals(char *s1, char *s2) {
  fprintf(stderr, "%s:%s on line %d\n", s1, s2, Line); exit(1);
}

void fatald(char *s, int d) {
  fprintf(stderr, "%s:%d on line %d\n", s, d, Line); exit(1);
}

void fatalc(char *s, int c) {
  fprintf(stderr, "%s:%c on line %d\n", s, c, Line); exit(1);
}
```

## まとめ

シンボルテーブル管理に始まり、2種のステートメントの追加、ASTノードタイプの追加、適切なx86-64アセンブリを生成するためのコードの追加など、多くの作業を終えました。

次回は6つの比較演算子を追加していきます。そうすれば制御構造に取り掛かることが可能となります。
