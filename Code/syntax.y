%{ 
	#include "tools.h"
	#include "lex.yy.c"
	extern int errorSyntaxFlag;
	extern int errorLexFlag;
	extern int yylex();
	int yyerror(char* msg);
	extern int yylineno;
	
	static ASTNode* make_node(const char* name,int lineno,int child_count)
	{
		if (child_count == 0) return NULL;
		ASTNode* node = ast_create_node(name, "", NODE_TYPE_NON_TERMINAL, lineno);
		return node;
	}
%}

%union {
    int type_int;
    float type_float;
    char* type_string;
    ASTNode* node;
};



/*--------------------High-level Definitions--------------------*/
%token <node> INT FLOAT
%token <node> LP RP LB RB LC RC SEMI COMMA
%token <node> RELOP
%token <node> ASSIGNOP PLUS MINUS STAR DIV AND OR DOT NOT
%token <node> STRUCT RETURN IF ELSE WHILE TYPE error
%token <node> ID

%right ASSIGNOP
%left OR
%left AND
%left RELOP 
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type <node> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier
%type <node> OptTag Tag VarDec FunDec VarList ParamDec CompSt
%type <node> StmtList Stmt DefList Def DecList Dec Exp Args

%%

/*--------------------High-level Definitions--------------------*/
Program : ExtDefList {
        $$ = make_node("Program", $1->lineno, 1);
        ast_add_child($$, 1, $1);
        ast_root = $$;
    }
;

ExtDefList : ExtDef ExtDefList {
        $$ = make_node("ExtDefList", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
|   /*empty*/ {
        $$ = NULL;
    }
;

ExtDef : Specifier ExtDecList SEMI {
        $$ = make_node("ExtDef", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Specifier SEMI {
        $$ = make_node("ExtDef", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
| Specifier FunDec CompSt {
        $$ = make_node("ExtDef", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Specifier FunDec SEMI {
        $$ = make_node("ExtDef", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| error SEMI {
    errorSyntaxFlag++;
}
|Specifier error SEMI {
    errorSyntaxFlag++;
}
;

ExtDecList : VarDec {
        $$ = make_node("ExtDecList", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| VarDec COMMA ExtDecList {
        $$ = make_node("ExtDecList", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
;

Specifier : TYPE  {
        $$ = make_node("Specifier", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| StructSpecifier  {
        $$ = make_node("Specifier", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
;

StructSpecifier : STRUCT OptTag LC DefList RC {
        $$ = make_node("StructSpecifier", $1->lineno, 5);
        ast_add_child($$, 5, $5, $4, $3, $2, $1);
    }
| STRUCT Tag {
        $$ = make_node("StructSpecifier", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
| error{
    errorSyntaxFlag++;
}
;

OptTag : ID {
        $$ = make_node("OptTag", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
|   /* empty */  {
        $$ = NULL;
    }
;

Tag : ID {
        $$ = make_node("Tag", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
;

VarDec : ID {
        $$ = make_node("VarDec", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| VarDec LB INT RB {
        $$ = make_node("VarDec", $1->lineno, 4);
        ast_add_child($$, 4, $4, $3, $2, $1);
    }
;

FunDec : ID LP VarList RP {
        $$ = make_node("FunDec", $1->lineno, 4);
        ast_add_child($$, 4, $4, $3, $2, $1);
    }
| ID LP RP {
        $$ = make_node("FunDec", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| ID LP error RP {
        errorSyntaxFlag++;
    }
| error LP VarList RP{
        errorSyntaxFlag++;
    };
;

VarList : ParamDec COMMA VarList {
        $$ = make_node("VarList", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| ParamDec {
        $$ = make_node("VarList", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
;

ParamDec : Specifier VarDec {
        $$ = make_node("ParamDec", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
;

CompSt : LC DefList StmtList RC {
        $$ = make_node("CompSt", $1->lineno, 4);
        ast_add_child($$, 4, $4, $3, $2, $1);
    }
| error RC {
        errorSyntaxFlag++;
    }
;

StmtList : Stmt StmtList {
        $$ = make_node("StmtList", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
| /* empty */ {
        $$ = NULL;
    }
;

Stmt : Exp SEMI {
        $$ = make_node("Stmt", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
| CompSt {
        $$ = make_node("Stmt", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| RETURN Exp SEMI {
        $$ = make_node("Stmt", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
        $$ = make_node("Stmt", $1->lineno, 5);
        ast_add_child($$, 5, $5, $4, $3, $2, $1);
    }
| IF LP Exp RP Stmt ELSE Stmt {
        $$ = make_node("Stmt", $1->lineno, 7);
        ast_add_child($$, 7, $7, $6, $5, $4, $3, $2, $1);
    }
| WHILE LP Exp RP Stmt {
        $$ = make_node("Stmt", $1->lineno, 5);
        ast_add_child($$, 5, $5, $4, $3, $2, $1);
    }
| Exp error {
        errorSyntaxFlag++;
    }

| error SEMI {
        errorSyntaxFlag++;
    }
;

DefList : Def DefList {
        $$ = make_node("DefList", $1->lineno, 2);
        ast_add_child($$, 2, $2, $1);
    }
| /* empty */  {
        $$ = NULL;
    }
;

Def : Specifier DecList SEMI {
        $$ = make_node("Def", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Specifier error SEMI {
        errorSyntaxFlag++;
    }
;

DecList : Dec {
        $$ = make_node("DecList", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| Dec COMMA DecList {
        $$ = make_node("DecList", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
;

Dec : VarDec {
        $$ = make_node("Dec", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| VarDec ASSIGNOP Exp {
        $$ = make_node("Dec", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
;

Exp : Exp ASSIGNOP Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp AND Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp OR Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp RELOP Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp PLUS Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp MINUS Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp STAR Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp DIV Exp {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| LP Exp RP {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| MINUS Exp {
        $$ = make_node("Exp", $1->lineno, 2);
        ast_add_child($$, 2, $2,$1);
    }
| NOT Exp {
        $$ = make_node("Exp", $1->lineno, 2);
        ast_add_child($$, 2, $2,$1);
    }
| ID LP Args RP {
        $$ = make_node("Exp", $1->lineno, 4);
        ast_add_child($$, 4, $4, $3, $2, $1);
    }
| ID LP RP {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp LB Exp RB {
        $$ = make_node("Exp", $1->lineno, 4);
        ast_add_child($$, 4, $4, $3, $2, $1);
    }
| Exp DOT ID {
        $$ = make_node("Exp", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| ID {
        $$ = make_node("Exp", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| INT {
        $$ = make_node("Exp", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| FLOAT {
        $$ = make_node("Exp", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
| Exp error{
    errorSyntaxFlag++;

}
| error{
    errorSyntaxFlag++;
}
;

Args : Exp COMMA Args {
        $$ = make_node("Args", $1->lineno, 3);
        ast_add_child($$, 3, $3, $2, $1);
    }
| Exp {
        $$ = make_node("Args", $1->lineno, 1);
        ast_add_child($$, 1, $1);
    }
;

%%

int yyerror(char* msg) {
    if ( current_error_line != yylineno) {
        printf("Error type B at Line %d: %s near %s.\n", yylineno, msg, yytext);
        current_error_line = yylineno;
    }
}


