#ifndef extern_
#define extern_ extern
#endif

// グローバル変数

extern_ int Line;       // 現在の行数
extern_ int Putback;    // スキャナによる文字の差し戻し
extern_ int Functionid; // 現在の関数のシンボルID
extern_ int Globs;      // グローバルシンボルスロットの次の空いている位置
extern_ FILE *Infile;   // 入出力ファイル
extern_ FILE *Outfile;
extern_ struct token Token;             // 最後にスキャンしたトークン
extern_ char Text[TEXTLEN + 1];         // 最後にスキャンした識別子
extern_ struct symtable Gsym[NSYMBOLS]; // グローバルシンボルテーブル

extern_ int O_dumpAST;
