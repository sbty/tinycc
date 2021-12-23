#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TEXTLEN 512   //  入力のシンボルの長さ
#define NSYMBOLS 1024 //  シンボルテーブルのエントリ数

// トークン
enum
{
  T_EOF,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_INTLIT,
  T_SEMI,
  T_EQUALS,
  T_IDENT,
  T_PRINT, // キーワード
  T_INT
};

struct token
{
  int token;    // 上のenumのいずれかの値を取る
  int intvalue; // tokenがT_INTLITであればその整数値
};

// ASTノード型
enum
{
  A_ADD,
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,
  A_INTLIT,
  A_IDENT,
  A_LVIDENT,
  A_ASSIGN
};

// AST構造体
struct ASTnode
{
  int op;               // このツリーで実行される操作
  struct ASTnode *left; // 左右の子ツリー
  struct ASTnode *right;
  int intvalue; // A_INTLITのときの整数値
  union
  {
    int intvalue; // A_INTLITの整数値
    int id;       // A_IDENTのシンボルスロット番号
  } v;
};

// シンボルテーブル構造体
struct symtable
{
  char *name; // シンボル名
};
