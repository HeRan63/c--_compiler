#ifndef SEMANTICTOOL_H
#define SEMANTICTOOL_H

#include "tools.h"

/* Global variables for semantics */
extern FunctionTable funcRegister;
extern HashTableNode currentScopeNode;
extern HashTableNode rootScopeNode;
extern HashTableNode_ symbolRegistry[TABLESIZE];
extern HashTableNode_ structRegistry[TABLESIZE];

extern SymbolTableNode* symbolTable;
extern unsigned tableSize;

/* Symbol table management functions */
HashTableNode createSymbolScopeTable();
SymbolTableNode buildSymbolEntry(Type typeInfo, char *symbolIdentifier, int defineStatus, int scopeLevel);
SymbolTableNode findSymbolInScope(char *name, int scope);
void registerSymbol(SymbolTableNode entry, HashTableNode scopeTable);
HashTableNode pushNewScope();
void popCurrentScope();
void removeSymbolFromTable(char *symbolIdentifier, int scopeLevel, HashTableNode targetScope);
SymbolTableNode constructSymbolEntry(Type typeInfo, char *symbolIdentifier, int category, bool defineStatus, int scopeLevel);
void trackFunctionDeclaration(char *funcName, int linePosition);
int addStructType(Type structTypeInfo, char *structIdentifier);
bool lookupLocalSymbol(Type *typeResult, char *symbolIdentifier, int *defineStatus, int scopeLevel, int visibilityMode);
bool lookupGlobalSymbol(Type *typeResult, char *symbolIdentifier, int *defineStatus, int scopeLevel, int *category);
SymbolTableNode findStructByName(char *structIdentifier);
bool checkStructExists(Type *typeResult, char *structIdentifier);
void reportSemanticError(enum SemanticError errorCode, int linePosition, char *msgDetails);
bool compareTypes(Type type1, Type type2);
bool compareArrayTypes(Type arrayType1, Type arrayType2);
int insertStruct(Type type, char *name, int offset, char *structName);
// 符号表操作函数
void initializeSymbolTable();
void insertSymbol(Type type, char* name, int kind, bool isDefined, int depth);
Type getSymbolType(char* name, int scope);
Type getStructureType(char* name);

// 类型检查函数
bool checkTypesMatch(Type t1, Type t2);
bool checkArrayTypesMatch(Type t1, Type t2);
bool checkFieldsMatch(FieldList f1, FieldList f2);

// 其他辅助函数
Type createBasicType(int basicType);
Type createArrayType(int size, Type elementType);
Type createStructType(char* name, FieldList structFields);
Type createFunctionType(int paramCount, FieldList params, Type returnType);
void cleanUp();

#endif /* SEMANTICTOOL_H */ 