#include "defs.h"
#include "data.h"
#include "decl.h"

// 字句解析

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
        return (c);
    }

    c = fgetc(Infile);
    if ('\n' == c)
        Line++;
    return (c);
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

// 入力ファイルをスキャンして整数リテラルを返す
// テキスト内の文字列として値を保存する
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
    return (val);
}

// 入力ファイルから識別子をスキャンし
// buf[]に保存する。識別子の長さを返す
static int scanident(int c, char *buf, int lim)
{
    int i = 0;

    // 数字、アルファベット、アンダースコアを受け付ける
    while (isalpha(c) || isdigit(c) || '_' == c)
    {
        // buf[]に追加して次の文字を取得
        // 識別子の長さの上限に到達したらエラー
        if (lim - 1 == i)
        {
            fatal("識別子が長すぎます");
        }
        else if (i < lim - 1)
        {
            buf[i++] = c;
        }
        c = next();
    }
    // 無効な文字であれば差し戻す
    // buf[]にNULL文字を追加して長さを返す
    putback(c);
    buf[i] = '\0';
    return (i);
}

// 入力からの単語を引数に取り、見つからなければ0、
// 見つかったら一致したキーワードのトークン番号を返す
// すべてのキーワードとstrcmp()を行う時間を減らすため
// 最初の文字でswitchによる分岐を使う
static int keyword(char *s)
{
    switch (*s)
    {

    case 'c':
        if (!strcmp(s, "char"))
            return (T_CHAR);
        break;
    case 'e':
        if (!strcmp(s, "else"))
            return (T_ELSE);
        break;
    case 'f':
        if (!strcmp(s, "for"))
            return (T_FOR);
        break;
    case 'i':
        if (!strcmp(s, "if"))
            return (T_IF);
        if (!strcmp(s, "int"))
            return (T_INT);
        break;
    case 'l':
        if (!strcmp(s, "long"))
            return (T_LONG);
        break;
    case 'p':
        if (!strcmp(s, "print"))
            return (T_PRINT);
        break;
    case 'r':
        if (!strcmp(s, "return"))
            return (T_RETURN);
        break;
    case 'w':
        if (!strcmp(s, "while"))
            return (T_WHILE);
        break;
    case 'v':
        if (!strcmp(s, "void"))
            return (T_VOID);
        break;
    }
    return (0);
}

// 拒否(排出)するトークンへのポインタ
static struct token *Rejtoken = NULL;

// スキャンしたばかりのトークンを拒否
void reject_token(struct token *t)
{
    if (Rejtoken != NULL)
        fatal("トークンを2回拒否できません");
    Rejtoken = t;
}

// スキャンを行い入力ファイルから見つかった次のトークンを返す。
// トークンが有効であれば１を、トークンが残っていなければ0を返す。
int scan(struct token *t)
{
    int c, tokentype;

    // 拒否されたトークンがあればそれを返す
    if (Rejtoken != NULL)
    {
        t = Rejtoken;
        Rejtoken = NULL;
        return (1);
    }

    //空白を飛ばす
    c = skip();

    //入力に応じてトークンを決める
    switch (c)
    {
    case EOF:
        t->token = T_EOF;
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
    case ';':
        t->token = T_SEMI;
        break;
    case '{':
        t->token = T_LBRACE;
        break;
    case '}':
        t->token = T_RBRACE;
        break;
    case '(':
        t->token = T_LPAREN;
        break;
    case ')':
        t->token = T_RPAREN;
        break;
    case ',':
        t->token = T_COMMA;
        break;
    case '=':
        if ((c = next()) == '=')
        {
            t->token = T_EQ;
        }
        else
        {
            putback(c);
            t->token = T_ASSIGN;
        }
        break;
    case '!':
        if ((c = next()) == '=')
        {
            t->token = T_NE;
        }
        else
        {
            fatalc("解釈できない文字", c);
        }
        break;
    case '<':
        if ((c = next()) == '=')
        {
            t->token = T_LE;
        }
        else
        {
            putback(c);
            t->token = T_LT;
        }
        break;
    case '>':
        if ((c = next()) == '=')
        {
            t->token = T_GE;
        }
        else
        {
            putback(c);
            t->token = T_GT;
        }
        break;

    case '&':
        if ((c = next()) == '&')
        {
            t->token = T_LOGAND;
        }
        else
        {
            putback(c);
            t->token = T_AMPER;
        }
        break;

    default:

        // 整数であればリテラルの整数値をスキャン
        if (isdigit(c))
        {
            t->intvalue = scanint(c);
            t->token = T_INTLIT;
            break;
        }
        else if (isalpha(c) || '_' == c)
        {
            // キーワードか識別子として読み取る
            scanident(c, Text, TEXTLEN);

            // 解釈できるキーワードであればそのトークンを返す
            if ((tokentype = keyword(Text)) != 0)
            {
                t->token = tokentype;
                break;
            }
            // 解釈できないキーワードなので識別子のはず
            t->token = T_IDENT;
            break;
        }

        fatalc("解釈できない文字", c);
    }

    // トークンが見つかった
    return (1);
}
