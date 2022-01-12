#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// 構造体とenum定義
#define TEXTLEN 512   //  入力のシンボルの長さ
#define NSYMBOLS 1024 //  シンボルテーブルのエントリ数

// トークン
enum
{
  T_EOF,
  T_PLUS, //
  T_MINUS,
  T_STAR, //
  T_SLASH,
  T_EQ, //
  T_NE,
  T_LT, //
  T_GT,
  T_LE,
  T_GE,
  T_INTLIT, //
  T_SEMI,
  T_ASSIGN,
  T_IDENT,
  T_LBRACE, //
  T_RBRACE,
  T_LPAREN,
  T_RPAREN,
  T_PRINT, // キーワード
  T_INT,
  T_IF,
  T_ELSE,
  T_WHILE,
  T_FOR,
  T_VOID
};

struct token
{
  int token;    // 上のenumのいずれかの値を取る
  int intvalue; // tokenがT_INTLITであればその整数値
};

// ASTノード型
enum
{
  A_ADD = 1, //
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,
  A_EQ, //
  A_NE,
  A_LT,
  A_GT,
  A_LE,
  A_GE,
  A_INTLIT, //
  A_IDENT,  //
  A_LVIDENT,
  A_ASSIGN,
  A_PRINT,
  A_GLUE,
  A_IF, //
  A_WHILE,
  A_FUNCTION
};

// AST構造体
struct ASTnode
{
  int op;               // このツリーで実行される操作
  struct ASTnode *left; // 左右の子ツリー
  struct ASTnode *mid;
  struct ASTnode *right;
  int intvalue; // A_INTLITのときの整数値
  union
  {
    int intvalue; // A_INTLITの整数値
    int id;       // A_IDENTのシンボルスロット番号
  } v;
};

#define NOREG -1 // AST生成関数が返すレジスタがないときに使う

// シンボルテーブル構造体
struct symtable
{
  char *name; // シンボル名
};
