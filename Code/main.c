#include "tools.h"
#include "semantic.h"
#include "intermediate.h"
#include "mips.h"
extern int yylineno;
ASTNode* ast_root;
int errorLexFlag;
int errorSyntaxFlag;
extern int yyparse(void);
extern void yyrestart(FILE*);

int main(int argc, char** argv) {
	//printf("main\n");
	if (argc <= 1) return 1;
	FILE *file1 = fopen(argv[1], "r");
	if (!file1)
	{
		perror(argv[1]);
		return 1;
	}
	

	FILE *file2 = fopen(argv[2], "wt+");
	if (!file2)
	{
		perror(argv[2]);
		return 1;
	}
	ast_root = NULL;
	errorLexFlag = 0;     // 词法错误标志
	errorSyntaxFlag = 0;  // 语法错误标志
	yylineno = 1;         // 初始化行号
	//printf("start\n");
	yyrestart(file1);
	//printf("restart\n");
	yyparse();
	//printf("parse\n");
	if (errorLexFlag == 0 && errorSyntaxFlag == 0) {
		//ast_print(ast_root, 0);  // 打印语法树
		
		// 进行语义分析
		AnalyzeProgram(ast_root);
		
		// 打印一下符号表状态
		printf("\n===== 语义分析后符号表状态 =====\n");
		for (int i = 0; i < TABLESIZE; i++) {
			SymbolTableNode entry = symbolRegistry[i].symbolTableNode;
			if (entry != NULL) {
				printf("哈希索引 %d:\n", i);
				while (entry) {
					printf("  符号: '%s', 深度: %d, 种类: %d, 已定义: %d\n", 
						entry->name ? entry->name : "NULL", 
						entry->depth, 
						entry->kind, 
						entry->isDefined);
					entry = entry->sameHashSymbolTableNode;
				}
			}
		}
		printf("===== 符号表状态结束 =====\n\n");
		
		// 进行中间代码生成
		printf("中间代码生成\n");
		ir_translate_program(ast_root, file2);
		generateMipsCode(file2);
		fclose(file2);
	}
	
	return 0;
}
