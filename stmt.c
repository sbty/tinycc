#include "defs.h"
#include "data.h"
#include "decl.h"

// ステートメントのパース

// statements: statement
//      | statement statements
//      ;
//
// statement: 'print' expression ';'
//      |     'int'   identifier ';'
//      |     identifier '=' expression ';'
//      ;
//
// identifier: T_IDENT
//      ;

void print_statement(void)
{
  struct ASTnode *tree;
  int reg;

  // printを最初のトークンとしてチェック
  match(T_PRINT, "print");

  // 続く式をパースしアセンブリを生成
  tree = binexpr(0);
  reg = genAST(tree, -1);
  genprintint(reg);
  genfreeregs();

  // セミコロンがあるか調べる
  semi();
}

void assignment_statement(void)
{
  struct ASTnode *left, *right, *tree;
  int id;

  // 識別子があるか調べる
  ident();

  // 定義されていたら葉ノードを作る
  if ((id = findglob(Text)) == -1)
  {
    fatals("変数が宣言されていません", Text);
  }
  right = mkastleaf(A_LVIDENT, id);

  // イコールがあるか調べる
  match(T_EQUALS, "=");

  // 続く式をパース
  left = binexpr(0);

  // 代入のASTを作る
  tree = mkastnode(A_ASSIGN, left, right, 0);

  // 代入のアセンブリコードを作る
  genAST(tree, -1);
  genfreeregs();

  // セミコロンが後ろにあるか調べる
  semi();
}

// 1つ以上のステートメントをパース
void statements(void)
{

  while (1)
  {
    switch (Token.token)
    {
    case T_PRINT:
      print_statement();
      break;
    case T_INT:
      var_declaration();
      break;
    case T_IDENT:
      assignment_statement();
      break;
    case T_EOF:
      return;
    default:
      fatald("構文エラー、 token", Token.token);
    }
  }
}
