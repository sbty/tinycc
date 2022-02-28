#include "defs.h"
#include "data.h"
#include "decl.h"

// 式をパース
// 1つの引数を伴う関数呼び出しをパースし
// ASTを返す
struct ASTnode *funccall(void)
{
  struct ASTnode *tree;
  int id;

  // 識別子が定義されているか調べる
  // それから葉ノードを作る
  if ((id = findglob(Text)) == -1)
  {
    fatals("宣言されていない関数です", Text);
  }
  // '(' を取得
  lparen();

  // 後続の式をパース
  tree = binexpr(0);

  // 関数呼び出しASTノードを作成
  // 関数の戻り値をノードの型として保存
  // ファンクションのシンボルidを記録
  tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);

  // ')' を取得
  rparen();
  return (tree);
}

// 主要な要素をパースして、ASTノードとして返すP
static struct ASTnode *primary(void)
{
  struct ASTnode *n;
  int id;

  // INTLITトークンであれば葉としてASTノードを作り
  // 次のトークンをスキャンする。それ以外はどのトークンタイプでも構文エラー
  switch (Token.token)
  {
  case T_INTLIT:
    // INTLITトークンであればAST葉ノードを作る
    // P_CHARの範囲内であればP_CHARで作る
    if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
      n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
    else
      n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
    break;

  case T_IDENT:
    // これは変数または関数呼び出し
    // 次のトークンをスキャンして判定
    scan(&Token);

    // '(' であれば関数呼び出し
    if (Token.token == T_LPAREN)
      return (funccall());

    // 関数呼び出しでないのでトークンを差し戻す
    reject_token(&Token);

    // 変数が宣言されているか調べる
    id = findglob(Text);
    if (id == -1)
      fatals("不明な変数", Text);

    // AST葉ノードを作る
    n = mkastleaf(A_IDENT, Gsym[id].type, id);
    break;

  default:
    fatald("構文エラー、トークン", Token.token);
  }

  // 次のトークンをスキャンして葉ノードを返す
  scan(&Token);
  return (n);
}

// 二項演算子トークンを二項演算子AST操作に変換する。
// トークンとAST操作は1:1の対応に依存する
static int binastop(int tokentype)
{
  if (tokentype > T_EOF && tokentype < T_INTLIT)
    return (tokentype);
  fatald("構文エラー token", tokentype);
  return (0); //
}

// トークンが右結合であればtrue、そうでなければfalseを返す
static int rightassoc(int tokentype)
{
  if (tokentype == T_ASSIGN)
    return (1);
  return (0);
}
// 各トークンのオペレータ優先順位
static int OpPrec[] = {
    0, 10,           // T_EOF, T_ASSIGN
    20, 20,          // PLUS, T_MINUS
    30, 30,          // T_STAR, T_SLASH
    40, 40,          // T_EQ, T_NE
    50, 50, 50, 50}; // T_LT, T_GT, T_LE, T_GE

// ２項演算子であることを確認してその優先順位を返す
static int op_precedence(int tokentype)
{
  int prec = OpPrec[tokentype];
  if (prec == 0)
    fatald("構文エラー トークン", tokentype);
  return (prec);
}

// prefix_expression: primary
//     | '*' prefix_expression
//     | '&' prefix_expression
//     ;

// プレフィクス式をパースしそのサブツリーを返す。
struct ASTnode *prefix(void)
{
  struct ASTnode *tree;
  switch (Token.token)
  {
  case T_AMPER:
    // 次のトークンを取得し、プレフィクス式として
    // 再帰的にパースする。
    scan(&Token);
    tree = prefix();

    // 識別子であるか確認する。
    if (tree->op != A_IDENT)
      fatal("& オペレータの後ろに識別子がありません。");

    // 操作をA_ADDRに、
    // 型を元の型を指すポインタに変更する。
    tree->op = A_ADDR;
    tree->type = pointer_to(tree->type);
    break;
  case T_STAR:
    // 次のトークンを取得し、プレフィクス式として
    // 再帰的にパースする。
    scan(&Token);
    tree = prefix();

    // とりあえず間接演算子か識別子か確認する。
    if (tree->op != A_IDENT && tree->op != A_DEREF)
      fatal("* オペレータの後ろに識別子も * もありません。");

    // ツリーの先頭にA_DEREF操作を付与。
    tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0);
    break;
  default:
    tree = primary();
  }
  return (tree);
}

// ルートが２項演算子であるASTツリーを返す。
// 引数ptpは１つ前のトークンの優先順位
struct ASTnode *binexpr(int ptp)
{
  struct ASTnode *left, *right;
  struct ASTnode *ltemp, *rtemp;
  int ASTop;
  int tokentype;

  // 左の整数リテラルを取得
  // 同時に次のトークンも取得
  left = prefix();

  // セミコロンか')'を見つけたら左ノードだけを返す。
  tokentype = Token.token;
  if (tokentype == T_SEMI || tokentype == T_RPAREN)
  {
    left->rvalue = 1;
    return (left);
  }

  // 1つ前のトークン(ptp)よりも現在のトークンの
  // 優先順位が高い限りループする。
  while ((op_precedence(tokentype) > ptp) ||
         (rightassoc(tokentype) && op_precedence(tokentype) == ptp))
  {
    //次の整数リテラルを取得
    scan(&Token);

    // 再帰的に現在のトークンの優先順位をbinexpr()に渡して
    // サブツリーを構築
    right = binexpr(OpPrec[tokentype]);

    // サブツリーに対して操作を実行するか判断する
    ASTop = binastop(tokentype);

    if (ASTop == A_ASSIGN)
    {
      // 代入
      // 右側のツリーを右辺値にする
      right->rvalue = 1;

      // 右の型が左と一致するか確認
      right = modify_type(right, left->type, 0);
      if (left == NULL)
        fatal("代入の式に互換性がありません");

      // 代入ASTツリーを作る。左と右を切り替えて右の式のコードが
      // 左の式より先に生成されるようにする。
      ltemp = left;
      left = right;
      right = ltemp;
    }
    else
    {

      // 代入を行っているわけではないので、両ツリーは右辺値となるはず。
      // 左辺値ツリーであった場合両ツリーを右辺値へと変換する。
      left->rvalue = 1;
      right->rvalue = 1;

      // 各ツリーがもう一方の型に合うか修正を試すことで
      // 互換性があるか確認する。
      ltemp = modify_type(left, right->type, ASTop);
      rtemp = modify_type(right, left->type, ASTop);
      if (ltemp == NULL && rtemp == NULL)
        fatal("型に互換性がありません");
      if (ltemp != NULL)
        left = ltemp;
      if (rtemp != NULL)
        right = rtemp;
    }
    // サブツリーを結合する。
    // 同時にトークンをAST操作へ変換
    left = mkastnode(binastop(tokentype), left->type, left, NULL, right, 0);

    // 現在のトークンの詳細を更新
    // セミコロンか')'を見つけたら左ノードを返す
    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN)
      left->rvalue = 1;
    return (left);
  }
  // 優先順位が同じか低いものになったらツリーを返す
  left->rvalue = 1;
  return (left);
}
