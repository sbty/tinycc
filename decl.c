#include "defs.h"
#include "data.h"
#include "decl.h"

// 宣言のパース

// 今見ているトークンをパースし
// その基本的な型をenumの値で返す
int parse_type(int t)
{
  if (t == T_CHAR)
    return (P_CHAR);
  if (t == T_INT)
    return (P_INT);
  if (t == T_LONG)
    return (P_LONG);
  if (t == T_VOID)
    return (P_VOID);
  fatald("不正な型, token", t);
}

// variable_declaration: 'int' identifier ';'  ;
// 変数宣言のパース
void var_declaration(void)
{
  int id, type;

  // 変数の型を取得し、その後識別子を取得する
  type = parse_type(Token.token);
  scan(&Token);
  ident();

  // Textには識別子の名前が入っている
  // 既知の識別子として登録
  // アセンブリでその場所を生成
  id = addglob(Text, type, S_VARIABLE, 0);
  genglobsym(id);

  // 後続のセミコロンを取得
  semi();
}

// 今の所、関数宣言はかなり単純化された文法
// function_declaration: 'void' identifier '(' ')' compound_statement   ;

// 単純化された関数の宣言をパース
struct ASTnode *function_declaration(void)
{
  struct ASTnode *tree, *finalstmt;
  int nameslot, type, endlabel;

  // 変数の型を取得し、その後識別子を取得
  type = parse_type(Token.token);
  scan(&Token);
  ident();

  // エンドラベルのラベルidを取得、
  // 関数をシンボルテーブルに追加、
  // グローバルのFuncionidに関数のシンボルidをセット
  endlabel = genlabel();
  nameslot = addglob(Text, type, S_FUNCTION, endlabel);
  Functionid = nameslot;

  lparen();
  rparen();

  // 合成ステートメントのASTツリーを取得
  tree = compound_statement();

  // 関数の型がP_VOIDでなければ合成ステートメントの
  // 最後のAST操作がreturn文であったか確認する
  if (type != P_VOID)
  {
    finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstmt == NULL || finalstmt->op != A_RETURN)
      fatal("非void型の関数が値を返しません");
  }
  // 関数の名前枠と合成ステートメントサブツリーを
  // 持っているFUNCTIONノードを返す
  return (mkastunary(A_FUNCTION, type, tree, nameslot));
}
