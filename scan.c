#include "defs.h"
#include "data.h"
#include "decl.h"

// 文字列sの位置cを返し、見つからなければ-1を返す
static int chrpos(char *s, int c)
{
    char *p;

    p = strchr(s, c);
    return (p ? p - s : -1);
}

// 入力ファイルから次の文字を探す
static int next(void)
{
    int c;

    if (Putback)
    {
        c = Putback;
        Putback = 0;
        return c;
    }

    c = fgetc(Infile);
    if ('\n' == c)
        Line++;
    return c;
}

static void putback(int c)
{
    Putback = c;
}

// 処理する必要のない入力を飛ばす。i.e. 空白、改行など。
// 必要ない文字列の先頭の文字を返す。
static int skip(void)
{
    int c;

    c = next();
    while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c)
    {
        c = next();
    }

    return (c);
}

static int scanint(int c)
{
    int k, val = 0;

    // 各数字をintの値に変換する
    while ((k = chrpos("0123456789", c)) >= 0)
    {
        val = val * 10 + k;
        c = next();
    }

    // 数字以外は破棄
    putback(c);
    return val;
}

// スキャンを行い入力ファイルから見つかった次のトークンを返す。
// トークンが有効であれば１を、トークンが残っていなければ0を返す。
int scan(struct token *t)
{
    int c;

    //空白を飛ばす
    c = skip();

    //入力に応じてトークンを決める
    switch (c)
    {
    case EOF:
        return (0);
    case '+':
        t->token = T_PLUS;
        break;
    case '-':
        t->token = T_MINUS;
        break;
    case '*':
        t->token = T_STAR;
        break;
    case '/':
        t->token = T_SLASH;
        break;

    default:

        // 数値であれば整数値が含まれているかスキャン
        if (isdigit(c))
        {
            t->intvalue = scanint(c);
            t->token = T_INTLIT;
            break;
        }
    }
}
