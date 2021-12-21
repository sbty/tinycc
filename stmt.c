#include "defs.h"
#include "data.h"
#include "decl.h"

// Parsing of statements

// statements: statement
//      | statement statements
//      ;
//
// statement: 'print' expression ';'
//      ;

// 1つ以上のステートメントをパース
void statements(void)
{
  struct ASTnode *tree;
  int reg;

  while (1)
  {
    // printを最初のトークンとしてチェック
    match(T_PRINT, "print");

    // 続く式をパースしアセンブリを生成
    tree = binexpr(0);
    reg = genAST(tree);
    genprintint(reg);
    genfreeregs();

    // セミコロンが続くチェックしEOFであれば終了
    semi();
    if (Token.token == T_EOF)
      return;
  }
}
