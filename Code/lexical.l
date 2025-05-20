%{	
	#include <stdio.h>
	#include <stdlib.h>
	#include "tools.h"
	#include "syntax.tab.h"
	extern int errorLexFlag;
	int yycolumn = 1;
	int yylex();
	
	/* 添加新变量用于记录当前行的错误状态 */
	static int current_error_line = 0;
	
	/* 错误报告函数 */
	static void report_error(const char* msg, const char* text) {
		if (current_error_line != yylineno) {
			printf("Error type A at Line %d: %s '%s'\n", yylineno, msg, text);
			errorLexFlag = 1;
			current_error_line = yylineno;
		}
	}
	static void report_error2(const char* msg, const char* text) {
		if (current_error_line != yylineno) {
			printf("Error type B at Line %d: %s '%s'\n", yylineno, msg, text);
			errorLexFlag = 1;
			current_error_line = yylineno;
		}
	}
	
	/* 词法单元处理函数 */
	static ASTNode* create_token(const char* type, const char* value) {
		return ast_create_node(type, value ? value : "", NODE_TYPE_TOKEN, yylineno);
	}

	static int handle_token(const char* type, const char* value, int token_type) {
	 //printf("Token: type=%s, value=%s, line=%d\n", type, value ? value : "NULL", yylineno);
		yylval.node = create_token(type, value);
		return token_type;
	}
	
%}


%option yylineno

/* 基本定义 */
D           [0-9]
L           [a-zA-Z]
H           [0-9a-fA-F]
W           [ \t\r\n]+
ID          ({L}|_)({L}|_|{D})*

/* 数字 */
INT         ([1-9]{D}*|0)|0[xX]{H}+|0[0-7]+
FLOAT       {D}+\.{D}+|({D}+\.{D}*|\.{D}+)[eE][+-]?{D}+|{D}+[eE][+-]?{D}+

/* 错误数字 */
ERR_OCT     0[0-7]*[8-9]+{D}*
ERR_HEX     0[xX]{H}*[g-zG-Z]+[0-9a-zA-Z]*
ERR_FLOAT   ({D}+\.{D}*|\.{D}+)[eE]

/* 运算符和关键字 */
RELOP       >|<|>=|<=|==|!=

%%

{W}         { /* 忽略空白字符 */ }

"int"|"float" { return handle_token("TYPE", yytext, TYPE); }
"struct"    { return handle_token("STRUCT", NULL, STRUCT); }
"return"    { return handle_token("RETURN", NULL, RETURN); }
"if"        { return handle_token("IF", NULL, IF); }
"else"      { return handle_token("ELSE", NULL, ELSE); }
"while"     { return handle_token("WHILE", NULL, WHILE); }

{ID}        { return handle_token("ID", yytext, ID); }
{INT}       { return handle_token("INT", yytext, INT); }
{FLOAT}     { return handle_token("FLOAT", yytext, FLOAT); }

"("         { return handle_token("LP", NULL, LP); }
")"         { return handle_token("RP", NULL, RP); }
"["         { return handle_token("LB", NULL, LB); }
"]"         { return handle_token("RB", NULL, RB); }
"{"         { return handle_token("LC", NULL, LC); }
"}"         { return handle_token("RC", NULL, RC); }
";"         { return handle_token("SEMI", NULL, SEMI); }
","         { return handle_token("COMMA", NULL, COMMA); }
"."         { return handle_token("DOT", NULL, DOT); }

{RELOP}     { return handle_token("RELOP", yytext, RELOP); }
"="         { return handle_token("ASSIGNOP", NULL, ASSIGNOP); }
"+"         { return handle_token("PLUS", NULL, PLUS); }
"-"         { return handle_token("MINUS", NULL, MINUS); }
"*"         { return handle_token("STAR", NULL, STAR); }
"/"         { return handle_token("DIV", NULL, DIV); }
"&&"        { return handle_token("AND", NULL, AND); }
"||"        { return handle_token("OR", NULL, OR); }
"!"         { return handle_token("NOT", NULL, NOT); }
"//"        { char c; while ((c = input()) != '\n' && c != 0); }

"/*"        { 
    char c, last_c = 0;
    int start_line = yylineno;  // 记录开始行号
    int found_end = 0;          // 用于标记是否找到结束符
    
    while ((c = input()) != 0) {
        if (last_c == '*' && c == '/') {
            // 找到第一个 "*/"
            found_end = 1;
            break;
        }
        last_c = c;
    }
    
    if (!found_end) {
        // 到达文件末尾仍未找到匹配的 "*/"
        report_error2("Unterminated comment from line", "/*");
		
    }
}

"*/"        {
	printf("Error type B at Line %d: Unmatched '*/'\n", yylineno);
	errorLexFlag = 1;
			current_error_line = yylineno;

}


{ERR_OCT}   { report_error("Invalid octal number", yytext); }
{ERR_HEX}   { report_error("Invalid hexadecimal number", yytext); }
{ERR_FLOAT} { report_error("Invalid floating point number", yytext); }
[^\x00-\x7F]+ { report_error("Mysterious character", yytext); }
.             { report_error("Mysterious character", yytext); }

%%