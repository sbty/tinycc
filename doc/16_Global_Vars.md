# パート16: 正しいグローバル変数の宣言

ポインタにオフセットを追加する問題について検討すると約束しましたが、先に考える必要が有ります。そこでグローバル変数の宣言を関数宣言の外に移動させることにしました。実際のところ、
変数宣言のパースを関数内に残したままです。将来ローカル変数宣言に変更するつもりだからです。

また、同じ型の変数を同時に複数宣言できるよう、文法を拡張したいです。

```c
int x, y, z;
```

## 追加するBNF文法

関数と変数両方のグローバル宣言のためにBNF文法を追加します。

```c
 global_declarations : global_declarations
      | global_declaration global_declarations
      ;

 global_declaration: function_declaration | var_declaration ;

 function_declaration: type identifier '(' ')' compound_statement   ;

 var_declaration: type identifier_list ';'  ;

 type: type_keyword opt_pointer  ;

 type_keyword: 'void' | 'char' | 'int' | 'long'  ;

 opt_pointer: <empty> | '*' opt_pointer  ;

 identifier_list: identifier | identifier ',' identifier_list ;
```

`function_declaration`と`global_declaration`はどちらも`type`で始まります。これは`type_keyword`であり、0個以上の'*'トークンがつく`opt_pointer`が後ろに続きます。その後ろの`function_declaration`と`global_declaration`には1つの識別子が必ず続きます。

しかし、型の後ろには`var_declaration`には`identifier_list`が続き、これは','トークンで区切られた1つ以上の識別子です。`var_declaration`も','トークンで終わる必要が有りますが、`function_declaration`は`compound_statement`で終わり、`;`がありません。
