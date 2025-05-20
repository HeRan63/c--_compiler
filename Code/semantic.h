#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "tools.h"
#include "semantictool.h"

// Debug control
extern int DEBUG_LEVEL;  // 0: no debug, 1: basic, 2: detailed, 3: verbose
#define DEBUG_NONE 0
#define DEBUG_BASIC 1
#define DEBUG_DETAILED 2
#define DEBUG_VERBOSE 3

#define DEBUG_PRINT(level, ...) \
    if (DEBUG_LEVEL >= level) { \
        printf(__VA_ARGS__); \
    }
/* 创建read和write函数 */
void createIOFunctions();
/* Entry point for semantic analysis */
void AnalyzeProgram(ASTNode *astRoot);
/* Handle ExtDefList */
void ProcessExtDefList(ASTNode *node);
/* Handle ExtDef */
void ProcessExtDef(ASTNode *node);
/* Handle ExtDecList */
void ProcessExtDecList(ASTNode *node, Type typeInfo);
/* Handle Specifier */
Type ProcessSpecifier(ASTNode *node);
/* Handle VarDec */
FieldList ProcessVarDec(ASTNode *node, Type typeInfo);
/* Handle FunDec */
int ProcessFunctionDeclaration(ASTNode *node, Type returnType, HashTableNode scope, bool isDefinition);
/* Handle VarList */
FieldList ProcessParameterList(ASTNode *node, HashTableNode scope);
/* Handle ParamDec */
FieldList ProcessParameter(ASTNode *node);
/* Handle CompSt */
void ProcessCompoundStatement(ASTNode *node, HashTableNode scope, Type returnType);
/* Handle StmtList */
void ProcessStatementList(ASTNode *node, HashTableNode scope, Type returnType);
/* Handle Stmt */
void ProcessStatement(ASTNode *node, HashTableNode scope, Type returnType);
/* Handle DefList */
void ProcessDefinitionList(ASTNode *node, HashTableNode scope);
/* Handle Def */
void ProcessDefinition(ASTNode *node, HashTableNode scope);
/* Handle DecList */
void ProcessDeclarationList(ASTNode *node, HashTableNode scope, Type typeInfo);
/* Handle Dec */
void ProcessDeclaration(ASTNode *node, HashTableNode scope, Type typeInfo);
/* Handle Exp */
Type ProcessExpression(ASTNode *node);
/* Handle Args */
int ProcessArgumentList(ASTNode *node, FieldList formalParams);
/* Handle StructDef */
typedef struct FieldInfo_ {
    char *name;
    int lineNo;
} FieldInfo;

FieldList ProcessStructureDefinition(ASTNode *node, char *structName, FieldInfo *allFields, int *fieldCount);
/* Handle StructDec */
FieldList ProcessStructureDeclaration(ASTNode *node, Type typeInfo, char *structName);
FieldList StructDec(ASTNode *root, Type type);
FieldList StructDef(ASTNode *root, char *name, int curOffset, int *tmpOffset);
#endif /* SEMANTIC_H */
