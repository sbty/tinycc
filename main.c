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
    Globs = 0;
}

// 引数がおかしいときに使い方を表示
static void usage(char *prog)
{
    fprintf(stderr, "Usage: %s infile\n", prog);
    exit(1);
}

// メイン。引数を調べて、なければ使い方を表示
// 入力ファイルを開いてscanfileを呼びtokenを見ていく。
int main(int argc, char *argv[])
{

    if (argc != 2)
        usage(argv[0]);

    init();

    if ((Infile = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "%s を開けません:%s\n", argv[1], strerror(errno));
        exit(1);
    }

    // 出力ファイルの作成
    if ((Outfile = fopen("out.s", "w")) == NULL)
    {
        fprintf(stderr, "out.sを作成できませんでした%s\n", strerror(errno));
        exit(1);
    }

    // とりあえずvoid printint()を確実に定義する
    addglob("printint", P_CHAR, S_FUNCTION, 0);

    scan(&Token);          // 入力ファイルの最初のトークンを取得
    genpreamble();         // プレアンブルを出力
    global_declarations(); // グローバル宣言のパース
    genpostamble();        // ポストアンブルを出力
    fclose(Outfile);       // 出力ファイルを閉じて終了
    return (0);
}
