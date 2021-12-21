# 05 ステートメント

開発中の言語の構文に、"ちゃんとした"ステートメントを追加して行きます。

```c
  print 2 + 3 * 5;
  print 18 - 6/3 + 4*2;
```

空白は無視するので、1つのステートメントですべてのトークンが同じ行にある必要はありません。各ステートメントは`print`キーワードで始まり、セミコロンで終了します。これらは新規のトークンとなります。

## BNFによる構文の記述

これまでBNFでの式の記述は見てきました。printとセミコロンのBNF構文を定義してみましょう。

```c
statements: statement
     | statement statements
     ;

statement: 'print' expression ';'
     ;
```

入力ファイルは複数のステートメントから構成されます。ステートメントが1つだけの場合もあれば、複数の場合もあるでしょう。それぞれのステートメントは`print`キーワードから始まり、その後に式が、そしてセミコロンが続きます。

## 字句スキャナへの変更

上記の構文をパースするコードに取り掛かる前に、既存のコードに少々追加する必要があります。まずは字句スキャナです。

セミコロンへの対応は簡単です。`print`キーワードはどうでしょうか。開発が進むに連れ言語は多くのキーワードが用意され、変数用に識別子も必要となるでしょう。それらに対処しやすくなるようなコードが必要です。

`scan.c`に、SubCコンパイラから借りた次のコードを追加しました。アルファベット以外の文字に到達するまで、アルファベットをバッファに読みこみます。

```c
// 入力ファイルから識別子をスキャンしbuf[]に保存する
// 識別子の長さを返す
static int scanident(int c, char *buf, int lim) {
  int i = 0;

  // 数字とアルファベット、_を受け付ける
  while (isalpha(c) || isdigit(c) || '_' == c) {
    // buf[]を伸ばして次のキャラクターを取得
    // もしも識別子の長さ上限に到達したらエラー
    if (lim - 1 == i) {
      printf("識別子が長すぎます line %d\n", Line);
      exit(1);
    } else if (i < lim - 1) {
      buf[i++] = c;
    }
    c = next();
  }
  // 無効な文字が見つかったら差し戻す
  // buf[]の終端文字をNULLとして長さを返す
  putback(c);
  buf[i] = '\0';
  return (i);
}
```

キーワードを解釈する関数も必要です。方法としてはキーワードのリストを持ち、`scanident()`からのバッファに対してリストを1つずつ見ながら`strcmp()`で比較する、などがあるでしょう。SubCからのコードは最適化されており、`strcmp()`を呼ぶ前に最初の文字とマッチさせています。このスピードアップは何重ものキーワードがあるときの比較です。現時点ではこの最適化は必要ありませんが、後々のことを考えて入れました。

```c
// 入力からの単語を引数に取り、見つからなければ0、
// 見つかったら一致したキーワードのトークン番号を返す
// すべてのキーワードとstrcmp()を行う時間を減らすため
// 最初の文字でswitchによる分岐を使う
static int keyword(char *s) {
  switch (*s) {
    case 'p':
      if (!strcmp(s, "print"))
        return (T_PRINT);
      break;
  }
  return (0);
}
```

`scan()`のswitch文の最後にセミコロンとキーワードを解釈するために次のコードを足します。

```c
    case ';':
      t->token = T_SEMI;
      break;
    default:

      // 数であればその整数リテラル値をスキャン
      if (isdigit(c)) {
        t->intvalue = scanint(c);
        t->token = T_INTLIT;
        break;
      } else if (isalpha(c) || '_' == c) {
        // キーワードまたは識別子として読み取る
        scanident(c, Text, TEXTLEN);

        // 解釈可能なキーワードであればトークンを返す
        if (tokentype = keyword(Text)) {
          t->token = tokentype;
          break;
        }
        // 解釈できなければエラー
        printf("解釈できないシンボル %s on line %d\n", Text, Line);
        exit(1);
      }
      // 文字は解釈できないトークンであり、エラー
      printf("解釈できない文字 %c on line %d\n", c, Line);
      exit(1);
```

キーワードと識別子の保存用にグローバルに`Text`バッファを追加しました。

```c
#define TEXTLEN         512             // 入力からのシンボルの探さ
extern_ char Text[TEXTLEN + 1];         // 最後にスキャンした識別子
```

## 式パーサへの変更

これまでの入力ファイルには1つの式だけが含まれていました。したがって、(expr.c内の)`binexpr()`のPrattパーサコードでは以下のコードでパーサを抜けていました。

```c
// トークンが残ってなければ左ノードを返す
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);
```

新たな構文ではそれぞれの式はセミコロンで終わります。ということで式パーサのコードを`T_SEMI`トークンを探し当て、式パーサを抜けるよう変更する必要があります。

```c
// ルートが二項演算子であるASTツリーを返す
// ptpは１つ前のトークンの優先順位
struct ASTnode *binexpr(int ptp) {
  struct ASTnode *left, *right;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンを取得
  left = primary();

  // セミコロンが見つかった左ノードを返す
  tokentype = Token.token;
  if (tokentype == T_SEMI)
    return (left);

    while (op_precedence(tokentype) > ptp) {
      ...

    // 現在のトークンの詳細を更新
    // セミコロンが見つかったら左ノードを返す
    tokentype = Token.token;
    if (tokentype == T_SEMI)
      return (left);
    }
}
```

## コードジェネレータへの変更

`gen.c`の汎用コードジェネレータと`cg.c`のCPU依存のコードは分けておきたいです。つまりコンパイラの残りの部分は`gen.c`の関数だけを、`gen.c`は`cg.c`のコードだけを呼び出すということです。

このため、`gen.c`にいくつか新しい"フロントエンド"関数を定義しました。

```c
void genpreamble()        { cgpreamble(); }
void genpostamble()       { cgpostamble(); }
void genfreeregs()        { freeall_registers(); }
void genprintint(int reg) { cgprintint(reg); }
```

## ステートメント用パーサの追加

`stmt.c`を追加しました。これは主だったステートメントをパースするコードが記述されています。今は上で述べたような、ステートメントをBNF構文でパースする必要があります。これは１つの関数で行われます。再帰的な定義をループ処理へ変換しました。

```c
// 1つ以上のステートメントをパース
void statements(void) {
  struct ASTnode *tree;
  int reg;

  while (1) {
    // 'print'を最初のトークンとしてチェック
    match(T_PRINT, "print");

    // 続く式をパース、アセンブリコードを生成
    tree = binexpr(0);
    reg = genAST(tree);
    genprintint(reg);
    genfreeregs();

    // 続くセミコロンをチェック
    // EOFなら終了
    semi();
    if (Token.token == T_EOF)
      return;
  }
}
```

各ループでT_PRINTトークンを探します。それから式をパースする`binexpr()`を呼び出します。最後にT_SEMIトークンを探します。T_EOFトークンが続いていた場合ループを抜けます。

式ツリーを作り終えたあと、ツリーをアセンブリコードに変換する`gen.c`の処理が呼び出され、最終結果を出力するためにアセンブリの`printint()`関数を呼びます。

## ヘルパー関数

上記のコードでは新規ファイル`misc.c`に用意したヘルパー関数を使っています。

```c
// 現在のトークンがtであるか確認して次のトークンを取得する
// tでなければエラー
void match(int t, char *what) {
  if (Token.token == t) {
    scan(&Token);
  } else {
    printf("%s ではありませんでした line %d\n", what, Line);
    exit(1);
  }
}

// セミコロンか調べて次のトークンを取得
void semi(void) {
  match(T_SEMI, ";");
}
```

パーサの構文チェックはこれらから構成されています。後ほど構文チェックをより簡単にするため、`match()`関数を呼ぶための小さな関数を追加します。

## main()への変更

初期の入力ファイルに記述していた式は、`main()`から直接`binexpr()`を呼び出してパースされていました。

```c
  scan(&Token);                 // 入力ファイルから最初のトークンを取得
  genpreamble();                // プレアンブルを出力
  statements();                 // 入力されたステートメントをパース
  genpostamble();               // ポストアンブルを出力
  fclose(Outfile);              // 出力ファイルを閉じて終了
  exit(0);
```

## 試行

追加や変更はこれぐらいです。新しいコードを試してみます。入力はinput01です。

```c
print 12 * 3;
print
   18 - 2
      * 4; print
1 + 2 +
  9 - 5/2 + 3*5;
```

トークンが複数の行に分かれています。`make test`でコンパイル、実行しましょう。

```bash
$ make test
cc -o comp1 -g cg.c expr.c gen.c main.c misc.c scan.c stmt.c tree.c
./comp1 input01
cc -o out out.s
./out
36
10
25
```

## まとめ

言語に対して初めて"本当の"文法を追加しました。BNF記法で定義しましたが、再帰を使わずにループで実装するほうが簡単でした。ただ、近いうちに再帰によるパースが必要になるでしょう。

その過程で汎用コードジェネレータとCPU依存ジェネレータをはっきりと分けるため、スキャナを修正、キーワードと識別子のサポートを追加する必要がありました。

次回は変数を追加していきます。
