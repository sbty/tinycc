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

// メイン。引数を調べて、なければ使い方を表示
// 入力ファイルを開いてscanfileを呼びtokenを見ていく。
void main(int argc, char *argv[])
{
    struct ASTnode *n;

    if (argc != 2)
        usage(argv[0]);

    init();

    if ((Infile = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "%s を開けません\n", argv[1], strerror(errno));
        exit(1);
    }

    scan(&Token);                    // 入力ファイルの最初のトークンを取得
    n = binexpr(0);                  // 入力ファイルの式をパース
    printf("%d\n", interpretAST(n)); // 最終結果を計算
    exit(0);
}
