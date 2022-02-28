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
  // オペレータ
  T_ASSIGN,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_EQ,
  T_NE,
  T_LT,
  T_GT,
  T_LE,
  T_GE,
  // 型キーワード
  T_VOID,
  T_CHAR,
  T_INT,
  T_LONG,
  //構造上のトークン
  T_INTLIT,
  T_SEMI,
  T_IDENT,
  T_LBRACE,
  T_RBRACE,
  T_LPAREN,
  T_RPAREN,
  T_AMPER,
  T_LOGAND,
  // 他キーワード
  T_IF,
  T_ELSE,
  T_WHILE,
  T_FOR,
  T_RETURN
};

struct token
{
  int token;    // 上のenumのいずれかの値を取る
  int intvalue; // tokenがT_INTLITであればその整数値
};

// ASTノード型
enum
{
  A_ASSIGN = 1,
  A_ADD,
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,
  A_EQ,
  A_NE,
  A_LT,
  A_GT,
  A_LE,
  A_GE,
  A_INTLIT,
  A_IDENT,
  A_GLUE,
  A_IF,
  A_WHILE,
  A_FUNCTION,
  A_WIDEN,
  A_RETURN,
  A_FUNCCALL,
  A_DEREF,
  A_ADDR,
  A_SCALE

};

// primitive types
enum
{
  P_NONE,
  P_VOID,
  P_CHAR,
  P_INT,
  P_LONG,
  P_VOIDPTR,
  P_CHARPTR,
  P_INTPTR,
  P_LONGPTR
};

// AST構造体
struct ASTnode
{
  int op;               // このツリーで実行される操作
  int type;             // このツリーを生成した式の型
  int rvalue;           // ノードが右辺値であればtrue
  struct ASTnode *left; // 左右の子ツリー
  struct ASTnode *mid;
  struct ASTnode *right;
  union
  {
    int intvalue; // A_INTLITの整数値
    int id;       // A_IDENTのシンボルスロット番号
    int size;     // A_SCALE用。スケールするサイズ
  } v;
};

#define NOREG -1 // AST生成関数が返すレジスタがないときに使う
// 構造上の型
#define NOLABEL 0 // genAST()に渡すラベルがないときに使う
enum
{
  S_VARIABLE,
  S_FUNCTION
};

// シンボルテーブル構造体
struct symtable
{
  char *name;   // シンボル名
  int type;     // シンボルのprimitive type
  int stype;    // シンボルの構造上の型
  int endlabel; // S_FUNCTIONのため、エンドラベル
};
