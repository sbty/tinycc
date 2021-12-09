#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"
#include <errno.h>

// グローバル変数の初期化
static void init()
{
    Line = 1;
    Putback = '\n';
}

// 引数がおかしいときに使い方を表示
static void usage(char *prog)
{
    fprintf(stderr, "Usage: %s infile\n", prog);
    exit(1);
}

// トークンのリスト
char *tokstr[] = {"+", "-", "*", "/", "int", "intlit"};

// 入力ファイルの全トークンを見ていく
// 見つかったトークンの詳細を出力する
static void scanfile()
{
    struct token T;

    while (scan(&T))
    {
        printf("Token %s", tokstr[T.token]);
        if (T.token == T_INTLIT)
            printf(", value %d", T.intvalue);
        printf("\n");
    }
}

// メイン。引数を調べて、なければ使い方を表示
// 入力ファイルを開いてscanfileを呼びtokenを見ていく。
void main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv[0]);

    init();

    if ((Infile = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "%s を開けません\n", argv[1], strerror(errno));
        exit(1);
    }

    scanfile();
    exit(0);
}
