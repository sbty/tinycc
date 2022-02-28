#include "defs.h"
#include "data.h"
#include "decl.h"

// 宣言のパース
// global_declarations : global_declarations
//      | global_declaration global_declarations
//      ;
//
// global_declaration: function_declaration | var_declaration ;
//
// function_declaration: type identifier '(' ')' compound_statement   ;
//
// var_declaration: type identifier_list ';'  ;
//
// type: type_keyword opt_pointer  ;
//
// type_keyword: 'void' | 'char' | 'int' | 'long'  ;
//
// opt_pointer: <empty> | '*' opt_pointer  ;
//
// identifier_list: identifier | identifier ',' identifier_list ;
//

// 今見ているトークンをパースし
// その基本的な型をenumの値で返す
int parse_type(void)
{
  int type;
  switch (Token.token)
  {
  case T_VOID:
    type = P_VOID;
    break;
  case T_CHAR:
    type = P_CHAR;
    break;
  case T_INT:
    type = P_INT;
    break;
  case T_LONG:
    type = P_LONG;
    break;
  default:
    fatald("不正な型, token", Token.token);
  }

  // 1つ以上先にある'*'トークンをスキャンし
  // 対応するポインタの型を決める
  while (1)
  {
    scan(&Token);
    if (Token.token != T_STAR)
      break;
    type = pointer_to(type);
  }

  // スキャンした次のトークンはそのままにしておく
  return (type);
}

// variable_declaration: 'int' identifier ';'  ;
// 変数宣言のパース
void var_declaration(int type)
{
  int id;

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
struct ASTnode *function_declaration(int type)
{
  struct ASTnode *tree, *finalstmt;
  int nameslot, endlabel;

  // グローバル変数Textには識別子の名前が入っている。
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
    // 関数内にステートメントがなければエラー
    if (tree == NULL)
      fatal("非void型関数内にステートメントがありません");

    // 合成ステートメントの最後のAST操作がreturn文
    // であったか確認する。
    finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstmt == NULL || finalstmt->op != A_RETURN)
      fatal("非void型の関数が値を返しません");
  }
  // 関数の名前枠と合成ステートメントサブツリーを
  // 持っているFUNCTIONノードを返す
  return (mkastunary(A_FUNCTION, type, tree, nameslot));
}

// 変数化関数の1つ以上のグローバル宣言をパースする。
void global_declarations(void)
{
  struct ASTnode *tree;
  int type;

  while (1)
  {

    // 型と識別子の後ろを見て関数宣言の'('か、
    // 変数宣言の','または';'か確認する。
    // Textはident()の呼び出しにより中身が入っている。
    type = parse_type();
    ident();
    if (Token.token == T_LPAREN)
    {

      // 関数宣言をパースして
      // アセンブリコードを生成する。
      tree = function_declaration(type);
      if (O_dumpAST)
      {
        dumpAST(tree, NOLABEL, 0);
        fprintf(stdout, "\n\n");
      }
      genAST(tree, NOREG, 0);
    }
    else
    {

      // グローバルの変数宣言をパースする
      var_declaration(type);
    }

    // EOFについたら終了
    if (Token.token == T_EOF)
      break;
  }
}
