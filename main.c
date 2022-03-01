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
    O_dumpAST = 0;
}

// 引数がおかしいときに使い方を表示
static void usage(char *prog)
{
    fprintf(stderr, "Usage: %s [-T] infile\n", prog);
    exit(1);
}

// メイン。引数を調べて、なければ使い方を表示
// 入力ファイルを開いてscanfileを呼びtokenを見ていく。
int main(int argc, char *argv[])
{
    int i;

    init();

    // コマンドラインオプション
    for (i = 1; i < argc; i++)
    {
        if (*argv[i] != '-')
            break;
        for (int j = 1; argv[i][j]; j++)
        {
            switch (argv[i][j])
            {
            case 'T':
                O_dumpAST = 1;
                break;
            default:
                usage(argv[0]);
            }
        }
    }

    if (i >= argc)
        usage(argv[0]);

    if ((Infile = fopen(argv[i], "r")) == NULL)
    {
        fprintf(stderr, "%s を開けません:%s\n", argv[i], strerror(errno));
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
