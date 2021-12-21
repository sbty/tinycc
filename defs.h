#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TEXTLEN 512 //  入力のシンボルの長さ

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
  T_PRINT
};

struct token
{
  int token;
  int intvalue;
};

// ASTノード型
enum
{
  A_ADD,
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,
  A_INTLIT,
};

// AST構造体
struct ASTnode
{
  int op;               // このツリーで実行される操作
  struct ASTnode *left; // 左右の子ツリー
  struct ASTnode *right;
  int intvalue; // A_INTLITのときの整数値
};
