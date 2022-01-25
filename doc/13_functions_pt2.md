# 13 関数 パート2

今回は関数呼び出しと戻り値を追加していきます。以下の内容です。

- 実装済みの関数の定義
- 1つだけの引数を伴う関数呼び出し、今はまだ使えない
- 関数からの戻り値
- ステートメントと式としての関数呼び出しの利用
- void関数が値を返さないことと、非void関数が値を返すことの確認

## キーワードとトークンの追加

これまでは`int`は8バイト(64ビット)としてきましたが、Gccは`int`を4バイト(32ビット)として扱っています。そこで`long`型を導入することにしました。

- `char`は1バイト幅
- `int`は4バイト(32bits)幅
- `long`は8バイト(64bits)幅

値を返す機能も必要なので`long`と`return`のキーワードを用意し、トークン'T_LONG'、'T_RETURN'と紐付けます。

## 関数呼び出しのパース

とりあえず関数呼び出しに使うBNF構文は以下のものです。

```c
function_call: identifier '(' expression ')'   ;
```

関数は名前の後ろに括弧のペアがきます。括弧の中に1つの引数が必要です。これを式と単体のステートメントの両方で使えるようにしたいです。

そこで`expr.c`にある関数呼び出しパーサ、`funccall()`から取り掛かります。呼ばれたときに、識別子はすでにスキャンされ関数名はグローバル変数`Text`にあります。

```c
// 単一の式を引数とする関数呼び出しをパースし
// ASTを返す
struct ASTnode *funccall(void) {
  struct ASTnode *tree;
  int id;

  // 識別子が定義されているか調べ
  // それから葉ノードをつくる
  if ((id = findglob(Text)) == -1) {
    fatals("宣言されていない関数", Text);
  }
  // '(' を取得
  lparen();

  // 後続の式をパース
  tree = binexpr(0);

  // 関数呼び出しASTノードを作成
  // 関数の戻り値をノードの型として保存
  // 関数のシンボルIDを記録
  tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);

  // ')' を取得
  rparen();
  return (tree);
}
```

関数や変数が定義されたとき、シンボルテーブルは構造上の型S_FUNCTIONとS_VARIABLEでそれぞれマークされます。本来はここに識別子が本当にS_FUNCTIONか確認するコードを加えるべきです。

単項ASTノード、A_FUNCCALLを追加します。子は引数として渡す単体の式です。ノード内にある関数のシンボルidを保存し、関数の戻り値の型を記録します。

## 不要なトークン

パースに関して問題が有ります。次の2つを区別しなければなりません。

```c
x = fred + jim;
x = fred(5) + jim;
```

1つ先のトークンを見て'('があるか確認する必要が有ります。あるならば関数呼び出しになります。でえすがこれを行うと既存のトークンを失います。これを解決するため、不要なトークンを差し戻せるようスキャナを変更します。トークンを返すとき、新しいものではなく次のトークンを返すようにしました。`scan.c`の追加コードは以下になります。

```c
// 拒否されたトークンへのポインタ
static struct token *Rejtoken = NULL;

// スキャンしたばかりのトークンを拒否
void reject_token(struct token *t) {
  if (Rejtoken != NULL)
    fatal("トークンを2回拒否はできません。");
  Rejtoken = t;
}

// スキャンして入力から見つけたトークンを返す
// トークンが有効であれば1，トークンがなければ0を返す
int scan(struct token *t) {
  int c, tokentype;

  // 拒否したトークンがあればそれを返す
  if (Rejtoken != NULL) {
    t = Rejtoken;
    Rejtoken = NULL;
    return (1);
  }

  // 通常のスキャンを続ける
  ...
}
```

## 式としての関数呼び出し

`expr.c`を見ていきます。`primary()`で変数名と関数呼び出しを区別しなければなりません。追加コードは以下になります。

```c
// 主要素をパースしそれらを表すASTノードを返す
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  int id;

  switch (Token.token) {
    ...
    case T_IDENT:
      // 変数か関数呼び出しとなる
      // 判別するために次のトークンをスキャン
      scan(&Token);

      // '(' なら関数呼び出し
      if (Token.token == T_LPAREN)
        return (funccall());

      // 関数呼び出しでないならトークンを拒否
      reject_token(&Token);

      // 通常の変数パースを続ける
      ...
}
```

## ステートメントとしての関数呼び出し

関数をステートメントとして呼び出そうとするときにも同じ問題があります。次の2つを区別しなければなりません。

```c
  fred = 2;
  fred(18);
```

`stmt.c`に追加するステートメントのコードは上記のものと同じようになります。

```c
// 代入ステートメントをパースしそのASTを返す
static struct ASTnode *assignment_statement(void) {
  struct ASTnode *left, *right, *tree;
  int lefttype, righttype;
  int id;

  // 識別子があるか確認
  ident();

  // 変数または関数呼び出し
  // 次のトークンが'('であれば関数呼び出し
  if (Token.token == T_LPAREN)
    return (funccall());

  // 関数呼び出しではないので代入
  ...
}
```

ここで不要なトークンを拒否することなくやれているのは、次のトークンが必ず'='か'('であり、これが正しいとわかっているからパーサコードを書けています。

## return文のパース

BNFでは、return文は次のようになります。

```c
return_statement: 'return' '(' expression ')'  ;
```

パースは簡単で、'return'、'('、`binexpr()`の呼び出し、')'、以上です。それよりも問題は型のチェックと、全く返さないことを許容するかどうかです。

return文に到達したときに、実際にどの関数にいるのか知る必要が有ります。`data.h`にグローバル変数を追加しました。

```c
extern_ int Functionid; // 現在の関数のシンボルID
```

そして`decl.c`の`function_declaration()`で次のように設定します。

```c
struct ASTnode *function_declaration(void) {
  ...
  // 関数をシンボルテーブルに追加し
  // グローバルのFunctionidをセット
  nameslot = addglob(Text, type, S_FUNCTION, endlabel);
  Functionid = nameslot;
  ...
}
```

関数宣言に入るたびにFunctionidを設定することでreturn文のパースや意味解釈のチェックに戻ることができます。以下が`stmt.c`の`return_statement()`の追加コードです。

```c
// return文をパースしそのASTを返す
static struct ASTnode *return_statement(void) {
  struct ASTnode *tree;
  int returntype, functype;

  // 関数がP_VOIDを返す場合は値を返さない
  if (Gsym[Functionid].type == P_VOID)
    fatal("void関数からは値を返せません");

  // 'return' と '(' があるか確認
  match(T_RETURN, "return");
  lparen();

  // 後続をパース
  tree = binexpr(0);

  // 関数の型と互換性があるか確認
  returntype = tree->type;
  functype = Gsym[Functionid].type;
  if (!type_compatible(&returntype, &functype, 1))
    fatal("型に互換性がありません");

  // 必要があれば左を拡張
  if (returntype)
    tree = mkastunary(returntype, functype, tree, 0);

  // A_RETURNノードにつなげる
  tree = mkastunary(A_RETURN, P_NONE, tree, 0);

  // ')' を取得
  rparen();
  return (tree);
}
```

子ツリーの式を返すA_RETURN ASTノードを追加しました。`type_compatible()`を使って式が戻り値の型と一致するか確認し、必要であれば拡張します。

最後に、関数が本当にvoidで宣言されていたか確認します。本当ならこの関数内でreturn文は実行できません。

## 型の見直し

前回で`type_compatible()`を導入し、リファクタリングについて触れました。`long`型を追加したので今では必須となりました。そこで`type.c`の新バージョンを用意しました。前回の最後の解説を見直すのみいいかもしれません。

```c
// 2つの基本型を引数にとり
// 互換性があればtrue、そうでなければfalseを返す
// さらに一方をもう一方に合わせるために拡張する必要があれば
// A_WIDENか0を返す。
// onlyrightがtrueであれば左だけを右へ拡張する

int type_compatible(int *left, int *right, int onlyright) {
  int leftsize, rightsize;

  // 同じ型であれば互換性がある
  if (*left == *right) { *left = *right = 0; return (1); }
  // 各型のサイズを取得
  leftsize = genprimsize(*left);
  rightsize = genprimsize(*right);

  // 型のサイズが0であればどの型とも互換性がない
  if ((leftsize == 0) || (rightsize == 0)) return (0);

  // 必要なら型を拡張
  if (leftsize < rightsize) { *left = A_WIDEN; *right = 0; return (1);
  }
  if (rightsize < leftsize) {
    if (onlyright) return (0);
    *left = 0; *right = A_WIDEN; return (1);
  }
  // 残りは同じサイズであるため互換性がある
  *left = *right = 0;
  return (1);
}
```

汎用コード生成で`genprimesize()`を呼び出し、`cg.c`の`cgprimesize`を呼んで型のサイズを取得しています。

```c
// P_XXX で並ぶ型のサイズの配列
// 0 はサイズなし P_NONE, P_VOID, P_CHAR, P_INT, P_LONG
static int psize[] = { 0,       0,      1,     4,     8 };

// P_XXX型の値を引数にとり、基本型のサイズをバイトで返す
int cgprimsize(int type) {
  // 型が有効か調べる
  if (type < P_NONE || type > P_LONG)
    fatal("cgprimsize()に不正な型が渡されました");
  return (psize[type]);
}
```

これにより型のサイズはプラットフォーム依存となり、別のプラットフォームでは異なる型サイズとなりえます。おそらく`P_INTLIT`を`int`ではなく`char`としているのは、リファクタリングされるべきかもしれません。

```c
if ((Token.intvalue) >= 0 && (Token.intvalue < 256))if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
```

## 非void関数は必ず値を返すようにする

void関数が値を返さないよう確認しました。次はどうやって非void関数に常に値を返させるかです。そのためには、関数の最後のステートメントがreturn文であることを確認する必要が有ります。

`decl.c`にある`function_declaration()`の終わりで次のようにしています。

```c
struct ASTnode *tree, *finalstmt;
  ...
  // 関数の型がP_VOIDでなければ合成ステートメントの
  // 最後のAST操作がreturn文であったか確認する
  if (type != P_VOID) {
    finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstmt == NULL || finalstmt->op != A_RETURN)
      fatal("非void型の関数が値を返していません。");
  }
```

関数が1つのステートメントしか持たないのであればA_GLUE ASTノードはなく、ツリーは合成ステートメントである左の子しかも持たないことになります。

この時点で以下のことができます。

- 関数の宣言、その型の保存、その関数内にいる記録
- 1つの引数を伴う関数呼び出し(式あるいはステートメント)
- 非void関数(のみ)からのreturn、非void関数内の最後のステートメントをreturn文に強制
- 返される式が関数の型定義と一致するか調べ、拡張する

ASTツリーにreturn分と関数呼び出しのためのA_RETURNとA_FUNCCALLノードが追加されました。これらがどのようにアセンブリを出力するか見ていきます。

## 引数が1つである理由

`print x;`を関数呼び出し`printint(x);`と置き換えたいからです。これにより本物のCの関数`printint()`をコンパイルしコンパイラの出力とリンクすることができます。

## 追加されたASTノード

`gen.c`の`genAST()`に追加されたコードはそれほど多くありません。

```c
    case A_RETURN:
      cgreturn(leftreg, Functionid);
      return (NOREG);
    case A_FUNCCALL:
      return (cgcall(leftreg, n->v.id));
```

A_RETURN は式でないため値を返しません。A_FUNCCALLはもちろん式です。

## x86-64出力の変更

コード生成に追加された内容は`cg.c`内にあるプラットフォーム依存のコード生成の箇所です。

### 追加された型

はじめに、現段階で`char`、`int`、`long`があり、x86-64は我々に各型に対して適切なレジスタ名を使うよう求めます。

```c
// 利用可能なレジスタとその名前のリスト
static int freereg[4];
static char *reglist[4] = { "%r8", "%r9", "%r10", "%r11" };
static char *breglist[4] = { "%r8b", "%r9b", "%r10b", "%r11b" };
static char *dreglist[4] = { "%r8d", "%r9d", "%r10d", "%r11d" }
```

### 変数の定義、読み込み、保存

変数は3種の型を持てるようになりました。生成されるコードはこれを反映しなければなりません。変更のある関数は以下のとおりです。

```c
// Generate a global symbol
void cgglobsym(int id) {
  int typesize;
  // Get the size of the type
  typesize = cgprimsize(Gsym[id].type);

  fprintf(Outfile, "\t.comm\t%s,%d,%d\n", Gsym[id].name, typesize, typesize);
}

// 変数からレジスタへ値を読み込む
// レジスタ番号を返す
int cgloadglob(int id) {
  // 新しくレジスタを取得
  int r = alloc_register();

  // レジスタを初期化するコードを出力
  switch (Gsym[id].type) {
    case P_CHAR:
      fprintf(Outfile, "\tmovzbq\t%s(\%%rip), %s\n", Gsym[id].name,
              reglist[r]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmovzbl\t%s(\%%rip), %s\n", Gsym[id].name,
              reglist[r]);
      break;
    case P_LONG:
      fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
      break;
    default:
      fatald("cgloadglob:型が不正です", Gsym[id].type);
  }
  return (r);
}

// レジスタの値を変数に保存
int cgstorglob(int r, int id) {
  switch (Gsym[id].type) {
    case P_CHAR:
      fprintf(Outfile, "\tmovb\t%s, %s(\%%rip)\n", breglist[r],
              Gsym[id].name);
      break;
    case P_INT:
      fprintf(Outfile, "\tmovl\t%s, %s(\%%rip)\n", dreglist[r],
              Gsym[id].name);
      break;
    case P_LONG:
      fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], Gsym[id].name);
      break;
    default:
      fatald("cgloadglob:不正な型です", Gsym[id].type);
  }
  return (r);
}
```

### 関数の呼び出し

1つの引数をつけて関数を呼ぶには、引数の値が入ったレジスタを`%rdi`へコピーする必要が有ります。戻り値では、%raxから返ってきた値を新しい値を入れるレジスタへコピーする必要が有ります。

```c
// 引数として与えたレジスタで1つの引数を伴う関数を呼び出す
// 結果の入ったレジスタを返す
int cgcall(int r, int id) {
  // 新しくレジスタを取得
  int outr = alloc_register();
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\t%s\n", Gsym[id].name);
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
  free_register(r);
  return (outr);
}
```

### 関数の戻り値

関数の実行中どの地点からでも値を返すには、関数の終わりにあるラベルへジャンプする必要が有ります。`function_declaration()`にラベルを作り、シンボルテーブルにそれを保存するコードを追加しました。戻り値は`%rax`レジスタに残るので、最後のラベルへジャンプする前にこのレジスタへコピーする必要があります。

```c
// 関数からの戻り値を生成するコード
void cgreturn(int reg, int id) {
  // 生成するコードは関数の型に依存する
  switch (Gsym[id].type) {
    case P_CHAR:
      fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[reg]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[reg]);
      break;
    case P_LONG:
      fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
      break;
    default:
      fatald("cgreturn:関数の型が不正です", Gsym[id].type);
  }
  cgjump(Gsym[id].endlabel);
}
```

### 関数プレアンブルとポストアンブルへの変更

プレアンブルには変更ありませんが、以前は値を返す際に`%rax`に0をセットしていました。これを削除する必要が有ります。

```c
// 関数のポストアンブルを出力
void cgfuncpostamble(int id) {
  cglabel(Gsym[id].endlabel);
  fputs("\tpopq %rbp\n" "\tret\n", Outfile);
}
```

### 初期プレアンブルへの変更

これまで手動で`printint()`のアセンブリを出力の先頭に挿入していました。これはもう必要ありません。本物のCの関数`printint()`をコンパイルできるようになったので、コンパイラからの出力とリンクさせればいいのです。

## 変更点のテスト

`tests/input14`を用意しました。

```c
int fred() {
  return(20);
}

void main() {
  int result;
  printint(10);
  result= fred(15);
  printint(result);
  printint(fred(15)+10);
  return(0);
}
```

最初に10を出力し、それから20を返す`fred()`を予備、それを出力します。再度`fred()`を呼び戻り値に10を足して30とします。これは1つの引数による関数呼び出しとその戻り値の説明です。結果は以下になります。

```bash
cc -o comp1 -g cg.c decl.c expr.c gen.c main.c misc.c scan.c
    stmt.c sym.c tree.c types.c
./comp1 tests/input14
cc -o out out.s lib/printint.c
./out; true
10
20
30
```

アセンブリの出力を`lib/printint.c`とリンクしています。

```c
#include <stdio.h>
void printint(long x) {
  printf("%ld\n", x);
}
```

## Cに

この変更により、次のことが可能となりました。

```bash
$ cat lib/printint.c tests/input14 > input14.c
$ cc -o out input14.c
$ ./out
10
20
30
```

つまりCのサブセットとしては十分でありCの関数をコンパイルして実行ファイルを手に入れることが可能です。

## まとめ

関数呼び出しと返り値の簡易バージョンに加えて、新しいデータ型も追加しました。予想通り簡単ではありませんでしたが、変更は妥当なものだったと思います。

次回は新たなハードウェアプラットフォーム、Raspberry PiのARM CPUへ対応していきます。
