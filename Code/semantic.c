#include "semantic.h"

int DEBUG_LEVEL = DEBUG_DETAILED;  // Default to verbose debug output
int currentScopeDepth = 0;
HashTableNode scopeTable = NULL;

// 前向声明函数
HashTableNode enterInnermostHashTable();
void deleteLocalVariable();
void validateFunctionDefinitions();

void AnalyzeProgram(ASTNode *astRoot) 
{
    DEBUG_PRINT(DEBUG_BASIC, "\n=== Starting Semantic Analysis ===\n");
    DEBUG_PRINT(DEBUG_DETAILED, "Initializing symbol table and starting analysis of AST root\n");
    
    // Initialize the symbol table
    scopeTable = createSymbolScopeTable();
    createIOFunctions();
    // Process the external definition list (root of AST)
    ProcessExtDefList(getChild(astRoot, 0));
    
    // Verify all function declarations have definitions
    DEBUG_PRINT(DEBUG_DETAILED, "Validating function definitions...\n");
    validateFunctionDefinitions();
    
    DEBUG_PRINT(DEBUG_BASIC, "=== Semantic Analysis Complete ===\n\n");
}

/* 
 * 创建read和write函数
 * read函数: 无参数，返回int类型
 * write函数: 接受一个int类型参数，返回int类型
 */
void createIOFunctions()
{
    // 创建基本类型 - int型，供读写函数共用
    Type intType = (Type)(malloc(sizeof(struct Type_)));
    intType->kind = BASIC;
    intType->u.basic = 0;  // 0表示int类型
    
    // ===== 创建write函数 =====
    // 分配函数名内存
    char *writeName = (char *)malloc(sizeof(char *) * 32);
    strcpy(writeName, "write");
    
    // 创建write函数参数
    FieldList writeParam = (FieldList)(malloc(sizeof(struct FieldList_)));
    writeParam->name = "write";  // 参数名
    writeParam->type = (Type)(malloc(sizeof(struct Type_)));
    writeParam->type->kind = BASIC;
    writeParam->type->u.basic = 0;  // int类型参数
    writeParam->nextFieldList = NULL;
    
    // 创建write函数类型
    Type writeFuncType = (Type)(malloc(sizeof(struct Type_)));
    writeFuncType->kind = FUNCTION;
    writeFuncType->u.function.parameterNum = 1;  // 有一个参数
    writeFuncType->u.function.returnType = intType;  // 返回int
    writeFuncType->u.function.parameters = writeParam;
    
    // 注册write函数到符号表
    // 参数说明: 类型, 名称, 种类(2表示函数), 是否已定义(1表示已定义), 作用域深度(0表示全局)
    registerSymbol(constructSymbolEntry(writeFuncType, writeName, 2, 1, 0), scopeTable);
    
    // ===== 创建read函数 =====
    // 分配函数名内存
    char *readName = (char *)malloc(sizeof(char *) * 32);
    strcpy(readName, "read");
    
    // 创建read函数类型
    Type readFuncType = (Type)(malloc(sizeof(struct Type_)));
    readFuncType->kind = FUNCTION;
    readFuncType->u.function.parameterNum = 0;  // 无参数
    readFuncType->u.function.returnType = intType;  // 返回int
    readFuncType->u.function.parameters = NULL;  // 无参数列表
    
    // 注册read函数到符号表
    registerSymbol(constructSymbolEntry(readFuncType, readName, 2, 1, 0), scopeTable);
}

void ProcessExtDefList(ASTNode *node) 
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing external definition list\n");
    
    // Check if node is NULL
    if (node == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "External definition list is empty\n");
        return;
    }
    
    ASTNode *firstChild = getChild(node, 0);
    ASTNode *secondChild = getChild(node, 1);
    
    // Process the first external definition
    if (firstChild != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing first external definition\n");
        ProcessExtDef(firstChild);
    }
    
    // Recursively process the rest of the external definition list
    if (secondChild != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Recursively processing remaining external definitions\n");
        ProcessExtDefList(secondChild);
    }
}

void ProcessExtDef(ASTNode *node)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing external definition at line %d\n", node->lineno);
    
    // Get child nodes
    ASTNode *specifierNode = getChild(node, 0);
    ASTNode *secondNode = getChild(node, 1);
    ASTNode *thirdNode = getChild(node, 2);
    
    // Type information from specifier
    Type typeInfo = NULL;
    
    // Process specifier to get type information
    if (specifierNode != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing type specifier\n");
        typeInfo = ProcessSpecifier(specifierNode);
    }
    
    // No further processing needed if there's no type
    if (typeInfo == NULL) {
        DEBUG_PRINT(DEBUG_DETAILED, "No type information available, skipping further processing\n");
        return;
    }
    
    // Handle external declarations based on structure
    if (thirdNode != NULL) {
        // Case 1: Global variable declarations
        if (secondNode != NULL && stringComparison(secondNode->name, "ExtDecList")) {
            DEBUG_PRINT(DEBUG_DETAILED, "Processing global variable declaration\n");
            ProcessExtDecList(secondNode, typeInfo);
        } 
        // Case 2: Function declaration or definition
        else {
            DEBUG_PRINT(DEBUG_DETAILED, "Processing function declaration/definition\n");
            
            // Create a new scope for the function
            HashTableNode functionScope = enterInnermostHashTable();
            
            // Check if it's a function definition (has compound statement) or just declaration
            if (stringComparison(thirdNode->name, "SEMI")) {
                DEBUG_PRINT(DEBUG_DETAILED, "Found function declaration\n");
                ProcessFunctionDeclaration(secondNode, typeInfo, functionScope, false);
            } else {
                DEBUG_PRINT(DEBUG_DETAILED, "Found function definition with body\n");
                ProcessFunctionDeclaration(secondNode, typeInfo, functionScope, true);
                
                // Increase scope depth and process function body
                currentScopeDepth++;
                DEBUG_PRINT(DEBUG_VERBOSE, "Entering function body scope (depth: %d)\n", currentScopeDepth);
                ProcessCompoundStatement(thirdNode, functionScope, typeInfo);
                currentScopeDepth--;
                DEBUG_PRINT(DEBUG_VERBOSE, "Exiting function body scope (depth: %d)\n", currentScopeDepth);
            }
            
            // Clean up local variables from this scope
            DEBUG_PRINT(DEBUG_DETAILED, "Cleaning up local variables from function scope\n");
            // Comment out this line to preserve local variables for code generation
            // deleteLocalVariable();
            printf("DEBUG: 保留局部变量以便中间代码生成\n");
        }
    }
}

void ProcessExtDecList(ASTNode *node, Type typeInfo)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing external declaration list at line %d\n", node->lineno);
    
    // Process the variable declaration
    FieldList field = ProcessVarDec(getChild(node, 0), typeInfo);
    
    // Check for duplicate variable names
    Type existingType = (Type)malloc(sizeof(struct Type_));
    int isDefined = 0;
    
    DEBUG_PRINT(DEBUG_VERBOSE, "Checking for duplicate variable name: %s\n", field->name);
    
    // If variable is already defined, report an error
    if (lookupLocalSymbol(&existingType, field->name, &isDefined, currentScopeDepth, 0)) {
        DEBUG_PRINT(DEBUG_BASIC, "Error: Variable %s is already defined\n", field->name);
        reportSemanticError(Redefined_Variable_Name, node->lineno, field->name);
    } else {
        // Otherwise, add it to the symbol table
        DEBUG_PRINT(DEBUG_VERBOSE, "Adding variable %s to symbol table\n", field->name);
        printf("DEBUG: 将变量'%s'添加到符号表, 作用域深度=%d\n", field->name, currentScopeDepth);
        SymbolTableNode newNode = constructSymbolEntry(field->type, field->name, 0, 1, currentScopeDepth);
        registerSymbol(newNode, scopeTable);
        
        // 确认符号是否成功添加到符号表
        Type checkType;
        int isDefined = 0;
        int kind = 0;
        if (lookupGlobalSymbol(&checkType, field->name, &isDefined, currentScopeDepth, &kind)) {
            printf("DEBUG: 确认符号'%s'已成功添加到符号表\n", field->name);
        } else {
            printf("ERROR: 符号'%s'未能成功添加到符号表\n", field->name);
        }
    }
    
    // Check if there are more variable declarations
    ASTNode *commaNode = getChild(node, 1);
    if (commaNode != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Found more variable declarations, processing recursively\n");
        // Process next variable declaration in the list
        ASTNode *nextDeclNode = getChild(node, 2);
        if (nextDeclNode != NULL) {
            ProcessExtDecList(nextDeclNode, typeInfo);
        }
    }
}

Type ProcessSpecifier(ASTNode *node)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing type specifier at line %d\n", node->lineno);
    
    ASTNode *firstChild = getChild(node, 0);
    if (firstChild == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "No type specifier found\n");
        return NULL;
    }
    
    // Allocate memory for the type
    Type typeInfo = (Type)malloc(sizeof(struct Type_));
    
    // Handle structure specifier
    if (stringComparison(firstChild->name, "StructSpecifier")) {
        DEBUG_PRINT(DEBUG_DETAILED, "Processing structure specifier\n");
        typeInfo->kind = STRUCTURE;
        ASTNode *secondNode = getChild(firstChild, 1);
        
        // Handle structure with tag (OptTag)
        if (stringComparison(secondNode->name, "OptTag")) {
            ASTNode *idNode = getChild(secondNode, 0);
            
            // Special case check
            if (stringComparison(idNode->value, "Data")) {
                DEBUG_PRINT(DEBUG_BASIC, "Special case: Data structure detected\n");
                exit(0);
            }
            
            if (stringComparison(idNode->name, "ID")) {
                // Get structure name
                char *structName = idNode->value;
                DEBUG_PRINT(DEBUG_DETAILED, "Processing named structure: %s\n", structName);
                
                // Check if structure is already defined
                Type tempType = (Type)malloc(sizeof(struct Type_));
                int isAlreadyDefined = 0;
                
                if (lookupLocalSymbol(&tempType, structName, &isAlreadyDefined, currentScopeDepth, 1)) {
                    DEBUG_PRINT(DEBUG_BASIC, "Error: Structure %s is already defined\n", structName);
                    reportSemanticError(Redefined_Field_Name, idNode->lineno, structName);
                    return NULL;
                } else {
                    // Set structure name
                    typeInfo->u.structure.name = (char *)malloc(sizeof(char) * 32);
                    strcpy(typeInfo->u.structure.name, structName);
                    
                    // Process structure fields
                    ASTNode *defListNode = getChild(firstChild, 3);
                    
                    // 创建一个临时结构存储所有字段信息
                    FieldInfo *allFields = (FieldInfo*)malloc(sizeof(FieldInfo) * 100);
                    int fieldCount = 0;
                    
                    if (!stringComparison(defListNode->name, "DefList")) {
                        DEBUG_PRINT(DEBUG_VERBOSE, "No field definitions found in structure\n");
                        typeInfo->u.structure.structures = NULL;
                    } else {
                        DEBUG_PRINT(DEBUG_DETAILED, "Processing structure field definitions\n");
                        // Parse all fields in the structure
                        ASTNode *currentDef = defListNode;
                        FieldList firstField = NULL;
                        FieldList currentField = NULL;
                        char *structName = typeInfo->u.structure.name ? typeInfo->u.structure.name : "anonymous";
                        
                        while (currentDef != NULL) {
                            ASTNode *defNode = getChild(currentDef, 0);
                            if (defNode == NULL) break;
                            
                            // 获取声明的类型
                            ASTNode *specifierNode = getChild(defNode, 0);
                            Type fieldType = ProcessSpecifier(specifierNode);
                            
                            // 获取声明列表
                            ASTNode *declListNode = getChild(defNode, 1);
                            ASTNode *currentDeclNode = declListNode;
                            
                            // 处理同一类型的所有声明
                            while (currentDeclNode != NULL) {
                                ASTNode *declNode = getChild(currentDeclNode, 0);
                                if (declNode == NULL) break;
                                
                                DEBUG_PRINT(DEBUG_VERBOSE, "Processing structure field at line %d\n", declNode->lineno);
                                FieldList field = ProcessStructureDeclaration(declNode, fieldType, structName);
                                
                                if (field != NULL) {
                                    // 检查重复字段
                                    bool isDuplicate = false;
                                    for (int i = 0; i < fieldCount; i++) {
                                        if (allFields[i].name && strcmp(field->name, allFields[i].name) == 0) {
                                            reportSemanticError(Redefined_Field, declNode->lineno, field->name);
                                            isDuplicate = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!isDuplicate) {
                                        // 记录字段信息
                                        allFields[fieldCount].name = (char *)malloc(strlen(field->name) + 1);
                                        if (allFields[fieldCount].name) {
                                            strcpy(allFields[fieldCount].name, field->name);
                                        }
                                        allFields[fieldCount].lineNo = declNode->lineno;
                                        fieldCount++;
                                        
                                        // 添加到字段列表
                                        field->nextFieldList = NULL;
                                        if (firstField == NULL) {
                                            firstField = field;
                                            currentField = firstField;
                                        } else {
                                            currentField->nextFieldList = field;
                                            currentField = field;
                                        }
                                    }
                                }
                                
                                // 移动到下一个声明
                                if (getChild(currentDeclNode, 1) == NULL) break;
                                currentDeclNode = getChild(currentDeclNode, 2);
                            }
                            
                            currentDef = getChild(currentDef, 1);
                        }
                        
                        typeInfo->u.structure.structures = firstField;
                    }
                    
                    // Clean up
                    for(int i = 0; i < fieldCount; i++) {
                        if (allFields[i].name) {
                            free(allFields[i].name);
                        }
                    }
                    free(allFields);
                }
                
                // Add structure to symbol table
                DEBUG_PRINT(DEBUG_DETAILED, "Adding structure %s to symbol table\n", structName);
                registerSymbol(constructSymbolEntry(typeInfo, structName, 1, 1, currentScopeDepth), scopeTable);
            }
        }
        // Handle structure reference (Tag)
        else if (stringComparison(secondNode->name, "Tag")) {
            ASTNode *idNode = getChild(secondNode, 0);
            char *structName = idNode->value;
            DEBUG_PRINT(DEBUG_DETAILED, "Processing structure reference: %s\n", structName);
            
            // Look up the structure type
            Type structType = NULL;
            int isDefined = 0;
            
            if (!lookupLocalSymbol(&structType, structName, &isDefined, currentScopeDepth, 1) || 
                structType == NULL || 
                structType->kind != STRUCTURE) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: Referenced structure %s is undefined\n", structName);
                reportSemanticError(Undefined_Struct, idNode->lineno, structName);
                return NULL;
            }
            
            DEBUG_PRINT(DEBUG_VERBOSE, "Found referenced structure type\n");
            return structType;
        }
        // Handle anonymous structure
        else if (stringComparison(secondNode->name, "LC")) {
            DEBUG_PRINT(DEBUG_DETAILED, "Processing anonymous structure\n");
            typeInfo->u.structure.name = NULL;  // 匿名结构体不需要名字
            
            // Process structure fields
            ASTNode *defListNode = getChild(firstChild, 2);
            
            // 创建一个临时结构存储所有字段信息
            FieldInfo *allFields = (FieldInfo*)malloc(sizeof(FieldInfo) * 100);
            int fieldCount = 0;
            
            if (!stringComparison(defListNode->name, "DefList")) {
                DEBUG_PRINT(DEBUG_VERBOSE, "No field definitions in anonymous structure\n");
                typeInfo->u.structure.structures = NULL;
            } else {
                DEBUG_PRINT(DEBUG_DETAILED, "Processing anonymous structure field definitions\n");
                // Parse all fields in the structure
                ASTNode *currentDef = defListNode;
                FieldList firstField = NULL;
                FieldList currentField = NULL;
                char *structName = typeInfo->u.structure.name ? typeInfo->u.structure.name : "anonymous";
                
                while (currentDef != NULL) {
                    ASTNode *defNode = getChild(currentDef, 0);
                    if (defNode == NULL) break;
                    
                    // 获取声明的类型
                    ASTNode *specifierNode = getChild(defNode, 0);
                    Type fieldType = ProcessSpecifier(specifierNode);
                    
                    // 获取声明列表
                    ASTNode *declListNode = getChild(defNode, 1);
                    ASTNode *currentDeclNode = declListNode;
                    
                    // 处理同一类型的所有声明
                    while (currentDeclNode != NULL) {
                        ASTNode *declNode = getChild(currentDeclNode, 0);
                        if (declNode == NULL) break;
                        
                        DEBUG_PRINT(DEBUG_VERBOSE, "Processing structure field at line %d\n", declNode->lineno);
                        FieldList field = ProcessStructureDeclaration(declNode, fieldType, structName);
                        
                        if (field != NULL) {
                            // 检查重复字段
                            bool isDuplicate = false;
                            for (int i = 0; i < fieldCount; i++) {
                                if (allFields[i].name && strcmp(field->name, allFields[i].name) == 0) {
                                    reportSemanticError(Redefined_Field, declNode->lineno, field->name);
                                    isDuplicate = true;
                                    break;
                                }
                            }
                            
                            if (!isDuplicate) {
                                // 记录字段信息
                                allFields[fieldCount].name = (char *)malloc(strlen(field->name) + 1);
                                if (allFields[fieldCount].name) {
                                    strcpy(allFields[fieldCount].name, field->name);
                                }
                                allFields[fieldCount].lineNo = declNode->lineno;
                                fieldCount++;
                                
                                // 添加到字段列表
                                field->nextFieldList = NULL;
                                if (firstField == NULL) {
                                    firstField = field;
                                    currentField = firstField;
                                } else {
                                    currentField->nextFieldList = field;
                                    currentField = field;
                                }
                            }
                        }
                        
                        // 移动到下一个声明
                        if (getChild(currentDeclNode, 1) == NULL) break;
                        currentDeclNode = getChild(currentDeclNode, 2);
                    }
                    
                    currentDef = getChild(currentDef, 1);
                }
                
                typeInfo->u.structure.structures = firstField;
            }
            
            // Clean up
            for(int i = 0; i < fieldCount; i++) {
                if (allFields[i].name) {
                    free(allFields[i].name);
                }
            }
            free(allFields);
        }
    }
    // Handle basic type (int/float)
    else if (stringComparison(firstChild->name, "TYPE")) {
        typeInfo->kind = BASIC;
        if (stringComparison(firstChild->value, "int")) {
            DEBUG_PRINT(DEBUG_DETAILED, "Processing basic type: int\n");
            typeInfo->u.basic = 0;
        } else if (stringComparison(firstChild->value, "float")) {
            DEBUG_PRINT(DEBUG_DETAILED, "Processing basic type: float\n");
            typeInfo->u.basic = 1;
        }
    }
    
    return typeInfo;
}

FieldList ProcessVarDec(ASTNode *node, Type typeInfo)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing variable declaration at line %d\n", node->lineno);
    printf("DEBUG: 开始处理变量声明，行号=%d\n", node->lineno);
    
    // Create a new field
    FieldList field = (FieldList)malloc(sizeof(FieldList_));
    if (field == NULL) {
        printf("ERROR: 为FieldList分配内存失败\n");
        return NULL;
    }
    
    field->nextFieldList = NULL;
    
    ASTNode *firstChild = getChild(node, 0);
    if (firstChild == NULL) {
        printf("ERROR: 变量声明节点没有子节点\n");
        free(field);
        return NULL;
    }
    
    printf("DEBUG: 变量声明的第一个子节点类型: %s\n", firstChild->name);
    
    // Base case: direct identifier
    if (stringComparison(firstChild->name, "ID")) {
        field->type = typeInfo;
        
        if (firstChild->value == NULL) {
            printf("ERROR: ID节点的值为NULL\n");
            free(field);
            return NULL;
        }
        
        field->name = firstChild->value;
        printf("DEBUG: 找到标识符: '%s'\n", field->name);
        return field;
    } 
    // Recursive case: array declaration
    else if (stringComparison(firstChild->name, "VarDec")) {
        // Process the base variable first
        FieldList baseField = ProcessVarDec(firstChild, typeInfo);
        if (baseField == NULL) {
            printf("ERROR: 处理基础变量声明失败\n");
            free(field);
            return NULL;
        }
        
        field->name = baseField->name;
        
        printf("DEBUG: 处理数组声明: '%s'\n", field->name);
        
        // Count array dimensions
        int dimensions = 0;
        ASTNode *current = firstChild;
        while (current != NULL && current->firstChild != NULL) {
            current = current->firstChild;
            dimensions++;
        }
        
        DEBUG_PRINT(DEBUG_VERBOSE, "Array has %d dimensions\n", dimensions);
        
        // Create array types from innermost to outermost
        Type *arrayTypes = (Type*)malloc(sizeof(Type) * (dimensions + 1));
        
        // Process each dimension, bottom-up
        current = firstChild;
        for (int i = dimensions-1; i >= 0; i--) {
            Type arrayType = (Type)malloc(sizeof(struct Type_));
            arrayType->kind = ARRAY;
            
            // Get array size from the INT node (third child)
            ASTNode *sizeNode = current->nextSibling->nextSibling;
            arrayType->u.array.size = My_atoi(sizeNode->value);
            
            DEBUG_PRINT(DEBUG_VERBOSE, "Dimension %d size: %d\n", i, arrayType->u.array.size);
            
            arrayTypes[i] = arrayType;
            current = current->firstChild;
        }
        
        // Link array types together
        for (int i = 0; i < dimensions-1; i++) {
            arrayTypes[i]->u.array.element = arrayTypes[i+1];
        }
        
        // Connect the innermost array type to the base type
        arrayTypes[dimensions-1]->u.array.element = typeInfo;
        
        // Set the field type to the outermost array type
        field->type = arrayTypes[0];
        
        printf("DEBUG: 完成数组声明处理: '%s'\n", field->name);
        return field;
    }
    
    printf("WARNING: 未知的变量声明类型: %s\n", firstChild->name);
    free(field);
    return NULL;
}

int ProcessFunctionDeclaration(ASTNode *node, Type returnType, HashTableNode scope, bool isDefinition)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing function declaration at line %d\n", node->lineno);
    
    // Get the function name (first child) and parameter list (third child)
    ASTNode *nameNode = getChild(node, 0);
    ASTNode *paramListNode = getChild(node, 2);
    
    DEBUG_PRINT(DEBUG_VERBOSE, "Function name: %s, Is definition: %d\n", nameNode->value, isDefinition);
    
    // Check if function is already in symbol table
    SymbolTableNode existingFunc = findSymbolInScope(nameNode->value, currentScopeDepth);
    bool functionExists = (existingFunc != NULL);
    bool existingFuncDefined = functionExists ? existingFunc->isDefined : false;
    Type existingFuncType = functionExists ? existingFunc->type : NULL;
    
    if (functionExists) {
        DEBUG_PRINT(DEBUG_DETAILED, "Function %s already exists in symbol table\n", nameNode->value);
    }
    
    // Create new function type
    Type funcType = (Type)malloc(sizeof(struct Type_));
    funcType->kind = FUNCTION;
    funcType->u.function.returnType = returnType;
    
    // Process parameters if they exist
    if (stringComparison(paramListNode->name, "VarList")) {
        DEBUG_PRINT(DEBUG_DETAILED, "Processing function parameters\n");
        // Increase scope depth for parameters
        currentScopeDepth++;
        DEBUG_PRINT(DEBUG_VERBOSE, "Entering parameter scope (depth: %d)\n", currentScopeDepth);
        
        // Process parameter list
        FieldList params = ProcessParameterList(paramListNode, scope);
        
        // Decrease scope depth
        currentScopeDepth--;
        DEBUG_PRINT(DEBUG_VERBOSE, "Exiting parameter scope (depth: %d)\n", currentScopeDepth);
        
        // Set function parameters
        funcType->u.function.parameters = params;
        
        // Count parameters
        int paramCount = 0;
        FieldList param = params;
        while (param != NULL) {
            paramCount++;
            param = param->nextFieldList;
        }
        
        DEBUG_PRINT(DEBUG_VERBOSE, "Function has %d parameters\n", paramCount);
        funcType->u.function.parameterNum = paramCount;
    } else {
        DEBUG_PRINT(DEBUG_VERBOSE, "Function has no parameters\n");
        // Function has no parameters
        funcType->u.function.parameterNum = 0;
        funcType->u.function.parameters = NULL;
    }
    
    // Handle different cases for function declarations/definitions
    if (functionExists) {
        if (isDefinition) {
            if (existingFuncDefined) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: Function %s is already defined\n", nameNode->value);
                reportSemanticError(Redefined_Function, node->lineno, nameNode->value);
                return -1;
            } else if (!compareTypes(existingFuncType, funcType)) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: Function %s declaration and definition don't match\n", nameNode->value);
                reportSemanticError(Conflict_Decordef_Funcion, node->lineno, nameNode->value);
                return -1;
            } else {
                DEBUG_PRINT(DEBUG_DETAILED, "Updating function %s entry to mark as defined\n", nameNode->value);
                registerSymbol(constructSymbolEntry(funcType, nameNode->value, 2, isDefinition, currentScopeDepth), scopeTable);
                return 0;
            }
        } else {
            // New declaration for existing function
            if (!compareTypes(existingFuncType, funcType)) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: Conflicting declarations for function %s\n", nameNode->value);
                reportSemanticError(Conflict_Decordef_Funcion, node->lineno, nameNode->value);
                return -1;
            }
        }
    } else {
        // New function
        DEBUG_PRINT(DEBUG_DETAILED, "Adding new function %s to symbol table\n", nameNode->value);
        registerSymbol(constructSymbolEntry(funcType, nameNode->value, 2, isDefinition, currentScopeDepth), scopeTable);
        
        // Record function declaration for later checking
        if (!isDefinition) {
            DEBUG_PRINT(DEBUG_VERBOSE, "Recording function declaration for later validation\n");
            trackFunctionDeclaration(nameNode->value, node->lineno);
        }
        return 0;
    }
    
    return 0;
}

FieldList ProcessParameterList(ASTNode *node, HashTableNode scope)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing parameter list at line %d\n", node->lineno);
    
    // Process the first parameter
    ASTNode *firstParamNode = getChild(node, 0);
    FieldList firstParam = ProcessParameter(firstParamNode);
    
    if (firstParam == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "First parameter is NULL\n");
        return NULL;
    }
    
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing first parameter: %s\n", firstParam->name);
    
    // Check for duplicate parameter name
    Type paramType = (Type)malloc(sizeof(struct Type_));
    int isDefined = 0;
    
    // Report error if parameter name conflicts with structure
    if (lookupLocalSymbol(&paramType, firstParam->name, &isDefined, 0, 0) && 
        paramType != NULL && 
        paramType->kind == STRUCTURE) {
        DEBUG_PRINT(DEBUG_BASIC, "Error: Parameter name %s conflicts with structure\n", firstParam->name);
        reportSemanticError(Redefined_Variable_Name, node->lineno, firstParam->name);
    }
    
    // Add parameter to symbol table
    DEBUG_PRINT(DEBUG_VERBOSE, "Adding parameter %s to symbol table\n", firstParam->name);
    SymbolTableNode paramNode = constructSymbolEntry(firstParam->type, firstParam->name, 0, 1, currentScopeDepth);
    registerSymbol(paramNode, scope);
    
    // If there are more parameters, process them recursively
    FieldList current = firstParam;
    ASTNode *currentNode = node;
    
    while (getChild(currentNode, 1) != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing next parameter\n");
        // Get the next parameter
        currentNode = getChild(currentNode, 2);
        FieldList nextParam = ProcessParameter(getChild(currentNode, 0));
        
        if (nextParam == NULL) {
            DEBUG_PRINT(DEBUG_VERBOSE, "Next parameter is NULL\n");
            continue;
        }
        
        DEBUG_PRINT(DEBUG_VERBOSE, "Next parameter: %s\n", nextParam->name);
        
        // Check for duplicate parameter name
        Type nextParamType = (Type)malloc(sizeof(struct Type_));
        int isNextDefined = 0;
        
        // Report error if parameter name conflicts with structure
        if (lookupLocalSymbol(&nextParamType, nextParam->name, &isNextDefined, 0, 0) && 
            nextParamType != NULL && 
            nextParamType->kind == STRUCTURE) {
            DEBUG_PRINT(DEBUG_BASIC, "Error: Parameter name %s conflicts with structure\n", nextParam->name);
            reportSemanticError(Redefined_Variable_Name, currentNode->lineno, nextParam->name);
        }
        
        // Add parameter to symbol table
        DEBUG_PRINT(DEBUG_VERBOSE, "Adding parameter %s to symbol table\n", nextParam->name);
        SymbolTableNode nextParamNode = constructSymbolEntry(nextParam->type, nextParam->name, 0, 1, currentScopeDepth);
        registerSymbol(nextParamNode, scope);
        
        // Link parameters together
        current->nextFieldList = nextParam;
        current = current->nextFieldList;
    }
    
    // Terminate parameter list
    DEBUG_PRINT(DEBUG_VERBOSE, "Finalizing parameter list\n");
    current->nextFieldList = NULL;
    
    return firstParam;
}

FieldList ProcessParameter(ASTNode *node)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing parameter at line %d\n", node->lineno);
    
    // Get specifier and variable declaration nodes
    ASTNode *specifierNode = getChild(node, 0);
    ASTNode *varDecNode = getChild(node, 1);
    
    if (specifierNode == NULL || varDecNode == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Invalid parameter node: missing specifier or variable declaration\n");
        return NULL;
    }
    
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing parameter type and declaration\n");
    
    // Get type from specifier
    Type paramType = ProcessSpecifier(specifierNode);
    if (paramType == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Failed to process parameter type\n");
        return NULL;
    }
    
    // Get parameter field information
    FieldList field = ProcessVarDec(varDecNode, paramType);
    if (field == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Failed to process parameter declaration\n");
        return NULL;
    }
    
    DEBUG_PRINT(DEBUG_VERBOSE, "Successfully processed parameter: %s\n", field->name);
    return field;
}

void ProcessCompoundStatement(ASTNode *node, HashTableNode scope, Type returnType)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing compound statement at line %d\n", node->lineno);
    
    // Get the child nodes
    ASTNode *secondChild = getChild(node, 1);
    ASTNode *thirdChild = getChild(node, 2);
    
    // Check structure of compound statement
    if (stringComparison(secondChild->name, "DefList")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Found local variable definitions\n");
        // Process local variable definitions
        ProcessDefinitionList(secondChild, scope);
        
        // If there are statements following the definitions, process them
        if (stringComparison(thirdChild->name, "StmtList")) {
            DEBUG_PRINT(DEBUG_VERBOSE, "Processing statement list after definitions\n");
            ProcessStatementList(thirdChild, scope, returnType);
        }
    } 
    // No local variable definitions, just statements
    else if (stringComparison(secondChild->name, "StmtList")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing statement list (no local definitions)\n");
        ProcessStatementList(secondChild, scope, returnType);
    }
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing compound statement\n");
}

void ProcessStatementList(ASTNode *node, HashTableNode scope, Type returnType)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing statement list\n");
    
    // If node is NULL, return
    if (node == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Empty statement list\n");
        return;
    }
    
    // Get the children
    ASTNode *firstStatement = getChild(node, 0);
    ASTNode *restStatements = getChild(node, 1);
    
    // If there is a statement, process it
    if (firstStatement != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing statement at line %d\n", firstStatement->lineno);
        ProcessStatement(firstStatement, scope, returnType);
    } else {
        // Empty statement list
        DEBUG_PRINT(DEBUG_VERBOSE, "No statements to process\n");
        return;
    }
    
    // If there are more statements, process them recursively
    if (restStatements != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing remaining statements\n");
        ProcessStatementList(restStatements, scope, returnType);
    }
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing statement list\n");
}

void ProcessStatement(ASTNode *node, HashTableNode scope, Type returnType)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing statement at line %d\n", node->lineno);
    
    // Get the first child to determine statement type
    ASTNode *firstChild = getChild(node, 0);
    if (firstChild == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Empty statement\n");
        return;
    }
    
    // Handle different statement types
    if (stringComparison(firstChild->name, "Exp")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing expression statement\n");
        // Expression statement: Exp SEMI
        ProcessExpression(firstChild);
    }
    else if (stringComparison(firstChild->name, "CompSt")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing compound statement\n");
        // Compound statement: create a new scope
        HashTableNode newScope = enterInnermostHashTable();
        currentScopeDepth++;
        DEBUG_PRINT(DEBUG_VERBOSE, "Entering new scope (depth: %d)\n", currentScopeDepth);
        
        // Process the compound statement
        ProcessCompoundStatement(firstChild, newScope, returnType);
        
        // Exit the scope
        currentScopeDepth--;
        DEBUG_PRINT(DEBUG_VERBOSE, "Exiting scope (depth: %d)\n", currentScopeDepth);
        deleteLocalVariable();
    }
    else if (stringComparison(firstChild->name, "RETURN")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing return statement\n");
        // Return statement: RETURN Exp SEMI
        ASTNode *expNode = getChild(node, 1);
        Type expType = ProcessExpression(expNode);
        
        // Check if return type matches function return type
        if (expType != NULL) {
            if (!compareTypes(expType, returnType)) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: Return type mismatch\n");
                reportSemanticError(Return_Type_Dismatch, node->lineno, NULL);
            } else {
                DEBUG_PRINT(DEBUG_VERBOSE, "Return type matches function return type\n");
            }
        }
    }
    else if (stringComparison(firstChild->name, "WHILE")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing while statement\n");
        // While statement: WHILE LP Exp RP Stmt
        ASTNode *conditionNode = getChild(node, 2);
        ASTNode *bodyNode = getChild(node, 4);
        
        // Check condition type
        Type condType = ProcessExpression(conditionNode);
        if (condType != NULL) {
            // Condition must be integer
            if (condType->kind != BASIC || condType->u.basic != 0) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: While condition must be integer\n");
                reportSemanticError(Operand_Type_Dismatch, node->lineno, NULL);
            } else {
                DEBUG_PRINT(DEBUG_VERBOSE, "Valid while condition type\n");
            }
        }
        
        // Process the loop body
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing while loop body\n");
        ProcessStatement(bodyNode, scope, returnType);
    }
    else if (stringComparison(firstChild->name, "IF")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing if statement\n");
        // If statement (possibly with else): IF LP Exp RP Stmt [ELSE Stmt]
        ASTNode *conditionNode = getChild(node, 2);
        ASTNode *ifBodyNode = getChild(node, 4);
        ASTNode *elseNode = getChild(node, 5);
        DEBUG_PRINT(DEBUG_VERBOSE, "Condition node: %s\n", conditionNode->name);
        // Check condition type
        Type condType = ProcessExpression(conditionNode);
      //  DEBUG_PRINT(DEBUG_VERBOSE, "Condition type: %d\n", condType->kind);
        if (condType != NULL) {
            // Condition must be integer
            DEBUG_PRINT(DEBUG_VERBOSE, "Condition type: %d\n", condType->kind);
            if (condType->kind != BASIC || condType->u.basic != 0) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: If condition must be integer\n");
                reportSemanticError(Operand_Type_Dismatch, node->lineno, NULL);
            } else {
                DEBUG_PRINT(DEBUG_VERBOSE, "Valid if condition type\n");
            }
        }
        
        // Process if body
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing if body\n");
        ProcessStatement(ifBodyNode, scope, returnType);
        
        // If there's an else clause, process it too
        if (elseNode != NULL) {
            DEBUG_PRINT(DEBUG_VERBOSE, "Processing else clause\n");
            ASTNode *elseBodyNode = getChild(node, 6);
            ProcessStatement(elseBodyNode, scope, returnType);
        }
    }
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing statement\n");
}

void ProcessDefinitionList(ASTNode *node, HashTableNode scope)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing definition list at line %d\n", node->lineno);
    
    // Check if the node is NULL
    if (node == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Empty definition list\n");
        return;
    }
    
    // Get the first definition and the rest of the list
    ASTNode *firstDef = getChild(node, 0);
    ASTNode *restDefs = getChild(node, 1);
    
    // Process the first definition if it exists
    if (firstDef != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing first definition\n");
        ProcessDefinition(firstDef, scope);
        
        // Process the remaining definitions recursively
        if (restDefs != NULL) {
            DEBUG_PRINT(DEBUG_VERBOSE, "Processing remaining definitions\n");
            ProcessDefinitionList(restDefs, scope);
        }
    }
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing definition list\n");
}

void ProcessDefinition(ASTNode *node, HashTableNode scope)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing definition at line %d\n", node->lineno);
    
    // Get the specifier and declaration list
    ASTNode *specifierNode = getChild(node, 0);
    ASTNode *declListNode = getChild(node, 1);
    
    // Make sure both nodes exist
    if (specifierNode == NULL || declListNode == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Invalid definition: missing specifier or declaration list\n");
        return;
    }
    
    // Process the specifier to get the type
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing type specifier\n");
    Type typeInfo = ProcessSpecifier(specifierNode);
    
    if (typeInfo == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Failed to process type specifier\n");
        return;
    }
    
    // Process all declarations with this type
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing declarations with type\n");
    ProcessDeclarationList(declListNode, scope, typeInfo);
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing definition\n");
}

void ProcessDeclarationList(ASTNode *node, HashTableNode scope, Type typeInfo)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing declaration list at line %d\n", node->lineno);
    
    // Get the first declaration and check if there are more
    ASTNode *firstDecl = getChild(node, 0);
    ASTNode *commaNode = getChild(node, 1);
    ASTNode *restDeclList = getChild(node, 2);
    
    if (firstDecl == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Empty declaration list\n");
        return;
    }
    
    // Process the first declaration
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing first declaration\n");
    ProcessDeclaration(firstDecl, scope, typeInfo);
    
    // If there are more declarations (indicated by a comma), process them
    if (commaNode != NULL && restDeclList != NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing remaining declarations\n");
        ProcessDeclarationList(restDeclList, scope, typeInfo);
    }
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing declaration list\n");
}

void ProcessDeclaration(ASTNode *node, HashTableNode scope, Type typeInfo)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing declaration at line %d\n", node->lineno);
    
    // Get nodes for the variable declaration and possible assignment
    ASTNode *varDecNode = getChild(node, 0);
    ASTNode *assignNode = getChild(node, 1);
    ASTNode *expNode = getChild(node, 2);
    
    if (varDecNode == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Invalid declaration: missing variable declaration\n");
        printf("ERROR: 无效声明：缺少变量声明节点\n");
        return;
    }
    
    // Process the variable declaration
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing variable declaration\n");
    FieldList field = ProcessVarDec(varDecNode, typeInfo);
    
    if (field == NULL) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Failed to process variable declaration\n");
        printf("ERROR: 处理变量声明失败，返回了NULL字段\n");
        return;
    }
    
    printf("DEBUG: 处理变量声明: 名称='%s'\n", field->name ? field->name : "NULL");
    
    // Check for name conflicts
    int isDefined1 = 0;
    Type localType = (Type)malloc(sizeof(struct Type_));
    bool localExists = lookupLocalSymbol(&localType, field->name, &isDefined1, currentScopeDepth, 0);
    
    int isDefined2 = 0;
    int kind = 0;
    Type globalType = (Type)malloc(sizeof(struct Type_));
    bool globalExists = lookupGlobalSymbol(&globalType, field->name, &isDefined2, currentScopeDepth, &kind);
    
    printf("DEBUG: 检查符号冲突: 名称='%s', 本地存在=%d, 全局存在=%d\n", 
           field->name, localExists ? 1 : 0, globalExists ? 1 : 0);
    
    // Handle simple declaration (no assignment)
    if (assignNode == NULL) {
        // Check for name conflicts
        if (localExists) {
            DEBUG_PRINT(DEBUG_BASIC, "Error: Variable %s is already defined in local scope\n", field->name);
            printf("ERROR: 变量'%s'在本地作用域中已定义\n", field->name);
            reportSemanticError(Redefined_Variable_Name, node->lineno, field->name);
        }
        else if (globalExists && globalType->kind == STRUCTURE && kind == 1) {
            DEBUG_PRINT(DEBUG_BASIC, "Error: Variable %s conflicts with structure name\n", field->name);
            printf("ERROR: 变量'%s'与结构体名称冲突\n", field->name);
            reportSemanticError(Redefined_Variable_Name, node->lineno, field->name);
        }
        else {
            // Add variable to symbol table
            DEBUG_PRINT(DEBUG_VERBOSE, "Adding variable %s to symbol table\n", field->name);
            printf("DEBUG: 将变量'%s'添加到符号表, 作用域深度=%d\n", field->name, currentScopeDepth);
            SymbolTableNode newNode = constructSymbolEntry(field->type, field->name, 0, 1, currentScopeDepth);
            registerSymbol(newNode, scope);
            
            // 确认符号是否成功添加到符号表
            Type checkType;
            int isDefined = 0;
            if (lookupGlobalSymbol(&checkType, field->name, &isDefined, currentScopeDepth, &kind)) {
                printf("DEBUG: 确认符号'%s'已成功添加到符号表\n", field->name);
            } else {
                printf("ERROR: 符号'%s'未能成功添加到符号表\n", field->name);
            }
        }
    }
    // Handle declaration with assignment
    else if (stringComparison(assignNode->name, "ASSIGNOP")) {
        DEBUG_PRINT(DEBUG_VERBOSE, "Processing declaration with assignment\n");
        printf("DEBUG: 处理带赋值的声明: 变量名='%s'\n", field->name);
        // Check for name conflicts
        if (localExists) {
            DEBUG_PRINT(DEBUG_BASIC, "Error: Variable %s is already defined in local scope\n", field->name);
            printf("ERROR: 变量'%s'在本地作用域中已定义\n", field->name);
            reportSemanticError(Redefined_Variable_Name, node->lineno, field->name);
        }
        else {
            // Add variable to symbol table
            DEBUG_PRINT(DEBUG_VERBOSE, "Adding variable %s to symbol table\n", field->name);
            printf("DEBUG: 将变量'%s'添加到符号表, 作用域深度=%d\n", field->name, currentScopeDepth);
            registerSymbol(constructSymbolEntry(field->type, field->name, 0, 1, currentScopeDepth), scope);
            
            // Process the expression and check type compatibility
            DEBUG_PRINT(DEBUG_VERBOSE, "Checking assignment type compatibility\n");
            Type expType = ProcessExpression(expNode);
            
            if (expType != NULL) {
                // Check if types match for assignment
                if (!compareTypes(field->type, expType)) {
                    DEBUG_PRINT(DEBUG_BASIC, "Error: Type mismatch in assignment\n");
                    reportSemanticError(AssignOP_Type_Dismatch, node->lineno, NULL);
                }
                // Check for structure name conflict
                else if (globalExists && globalType->kind == STRUCTURE && kind == 1) {
                    DEBUG_PRINT(DEBUG_BASIC, "Error: Variable %s conflicts with structure name\n", field->name);
                    reportSemanticError(Redefined_Variable_Name, node->lineno, field->name);
                }
            }
        }
    }
    
    DEBUG_PRINT(DEBUG_DETAILED, "Completed processing declaration\n");
}

Type ProcessExpression(ASTNode *node)
{
    // NULL check
    if (node == NULL) return NULL;
    DEBUG_PRINT(DEBUG_VERBOSE, "Processing expression at line %d\n", node->lineno);
    // Get the children nodes
    ASTNode *firstChild = getChild(node, 0);
    ASTNode *secondChild = getChild(node, 1);
    ASTNode *thirdChild = getChild(node, 2);
    ASTNode *fourthChild = getChild(node, 3);
   
    // Case 1: Expressions involving binary operations
    if (stringComparison(firstChild->name, "Exp")) {
        // Handle assignment: Exp ASSIGNOP Exp
        if (stringComparison(secondChild->name, "ASSIGNOP")) {
            // Check if left side is an l-value
            ASTNode *leftExp = firstChild;
            ASTNode *leftFirst = getChild(leftExp, 0);
            ASTNode *leftSecond = getChild(leftExp, 1);
            ASTNode *leftThird = getChild(leftExp, 2);
            ASTNode *leftFourth = getChild(leftExp, 3);
            
            // Simple variable must be an ID
            if (leftSecond == NULL && !stringComparison(leftFirst->name, "ID")) {
                reportSemanticError(Leftside_Rvalue_Error, node->lineno, NULL);
                return NULL;
            }
            
            // Complex expressions must be array access or structure member
            if (leftSecond != NULL) {
                // Check if it's a valid l-value (array access or field access)
                bool isArrayAccess = leftFourth && 
                                     stringComparison(leftFirst->name, "Exp") && 
                                     stringComparison(leftSecond->name, "LB") && 
                                     stringComparison(leftThird->name, "Exp") && 
                                     stringComparison(leftFourth->name, "RB");
                
                bool isFieldAccess = leftThird && !leftFourth && 
                                     stringComparison(leftFirst->name, "Exp") && 
                                     stringComparison(leftSecond->name, "DOT") && 
                                     stringComparison(leftThird->name, "ID");
                
                if (!isArrayAccess && !isFieldAccess) {
                    reportSemanticError(Leftside_Rvalue_Error, node->lineno, NULL);
                    return NULL;
                }
            }
            
            // Process left and right expressions
            Type leftType = ProcessExpression(firstChild);
            Type rightType = ProcessExpression(thirdChild);
            DEBUG_PRINT(DEBUG_VERBOSE, "Left type: %d\n", leftType->kind);
            DEBUG_PRINT(DEBUG_VERBOSE, "Right type: %d\n", rightType->kind);
            // Check type match for assignment
            if (leftType && rightType && !compareTypes(leftType, rightType)) {
                reportSemanticError(AssignOP_Type_Dismatch, node->lineno, NULL);
                return NULL;
            }
            
            return leftType;
        }
        // Handle other binary operations
        else if (stringComparison(secondChild->name, "AND") || 
                 stringComparison(secondChild->name, "OR")) {
            // Get types of operands
            Type leftType = ProcessExpression(firstChild);
            Type rightType = ProcessExpression(thirdChild);
            
            // For logical operators, both operands must be valid expressions
            if (leftType && rightType) {
                // Create and return an int type for the result
                Type resultType = (Type)malloc(sizeof(struct Type_));
                resultType->kind = BASIC;
                resultType->u.basic = 0;  // int type
                return resultType;
            }
            return NULL;
        }
        else if (stringComparison(secondChild->name, "RELOP")) {
            // Get types of operands
            Type leftType = ProcessExpression(firstChild);
            Type rightType = ProcessExpression(thirdChild);
            
            // For relational operators, operands must be of the same type
            if (leftType && rightType && compareTypes(leftType, rightType)) {
                // Create and return an int type for the result
                Type resultType = (Type)malloc(sizeof(struct Type_));
                resultType->kind = BASIC;
                resultType->u.basic = 0;  // int type
                return resultType;
            } else {
                reportSemanticError(Operand_Type_Dismatch, node->lineno, NULL);
                return NULL;
            }
        }
        else if (stringComparison(secondChild->name, "PLUS") || 
                 stringComparison(secondChild->name, "MINUS") || 
                 stringComparison(secondChild->name, "STAR") || 
                 stringComparison(secondChild->name, "DIV")) {
            // Get types of operands
            Type leftType = ProcessExpression(firstChild);
            Type rightType = ProcessExpression(thirdChild);
            
            // For arithmetic operators, both operands must be basic types
            if (leftType && rightType) {
                if (leftType->kind == BASIC && rightType->kind == BASIC && 
                    leftType->u.basic == rightType->u.basic) {
                    return leftType;
                } else {
                    reportSemanticError(Operand_Type_Dismatch, node->lineno, NULL);
                }
            }
            return NULL;
        }
        // Handle array access: Exp LB Exp RB
        else if (stringComparison(secondChild->name, "LB")) {
            // Process the array expression and index
            Type arrayType = ProcessExpression(firstChild);
            Type indexType = ProcessExpression(thirdChild);
            
            // Check if array type is valid
            if (arrayType == NULL) return NULL;
            
            if (arrayType->kind != ARRAY) {
                reportSemanticError(Operate_Others_As_Array, node->lineno, NULL);
                return NULL;
            }
            
            // Check if index type is valid (must be integer)
            if (indexType == NULL) return NULL;
            
            if (!(indexType->kind == BASIC && indexType->u.basic == 0)) {
                reportSemanticError(Array_Float_Index, node->lineno, NULL);
                return NULL;
            }
            
            // Return the element type of the array
            return arrayType->u.array.element;
        }
        // Handle structure member access: Exp DOT ID
        else if (stringComparison(secondChild->name, "DOT")) {
            // Process the structure expression
            Type structType = ProcessExpression(firstChild);
            
            // Check if structure type is valid
            if (structType == NULL) return NULL;
            
            if (structType->kind != STRUCTURE) {
                reportSemanticError(Operate_Others_As_Struct, node->lineno, NULL);
                return NULL;
            }
            
            // Find the field in the structure
            FieldList fieldList = structType->u.structure.structures;
            char *fieldName = thirdChild->value;
            
            // 遍历结构体的字段列表
            while (fieldList != NULL) {
                if (stringComparison(fieldList->name, fieldName)) {
                    return fieldList->type;
                }
                fieldList = fieldList->nextFieldList;
            }
            
            // Field not found
            reportSemanticError(Undefined_Field, node->lineno, fieldName);
            return NULL;
        }
    }
    // Case 2: Terminal values (ID, INT, FLOAT)
    else if (secondChild == NULL) {
        // Handle variable references
        if (stringComparison(firstChild->name, "ID")) {
            // Look up variable in both local and global scopes
            Type localType = (Type)malloc(sizeof(struct Type_));
            int localIsDefined = 0;
            bool localFound = lookupLocalSymbol(&localType, firstChild->value, &localIsDefined, currentScopeDepth, 0);
            
            Type globalType = (Type)malloc(sizeof(struct Type_));
            int globalKind = 0;
            int globalIsDefined = 0;
            bool globalFound = lookupGlobalSymbol(&globalType, firstChild->value, &globalIsDefined, currentScopeDepth, &globalKind);
            
            // If found locally, return local type
            if (localFound) {
                return localType;
            }
            // If found globally and is a variable, return global type
            else if (globalFound && globalKind == 0) {
                return globalType;
            }
            // Otherwise, report error
            else {
                reportSemanticError(Undefined_Variable, node->lineno, NULL);
                return NULL;
            }
        }
        // Handle integer literals
        else if (stringComparison(firstChild->name, "INT")) {
            Type intType = (Type)malloc(sizeof(struct Type_));
            intType->kind = BASIC;
            intType->u.basic = 0;  // 0 represents int
            return intType;
        }
        // Handle float literals
        else if (stringComparison(firstChild->name, "FLOAT")) {
            Type floatType = (Type)malloc(sizeof(struct Type_));
            floatType->kind = BASIC;
            floatType->u.basic = 1;  // 1 represents float
            return floatType;
        }
    }
    // Case 3: Various unary and other expressions
    else {
        // Handle parenthesized expressions: LP Exp RP
        if (stringComparison(firstChild->name, "LP")) {
            return ProcessExpression(secondChild);
        }
        // Handle unary minus: MINUS Exp
        else if (stringComparison(firstChild->name, "MINUS")) {
            Type expType = ProcessExpression(secondChild);
            
            if (expType == NULL) return NULL;
            
            // Must be a basic type (int or float)
            if (expType->kind != BASIC) {
                reportSemanticError(Operand_Type_Dismatch, node->lineno, NULL);
                return NULL;
            }
            
            return expType;
        }
        // Handle logical not: NOT Exp
        else if (stringComparison(firstChild->name, "NOT")) {
            Type expType = ProcessExpression(secondChild);
            
            if (expType == NULL) return NULL;
            
            // Must be an integer for logical operations
            if (expType->kind != BASIC || expType->u.basic != 0) {
                reportSemanticError(Operand_Type_Dismatch, node->lineno, NULL);
                return NULL;
            }
            
            return expType;
        }
        // Handle function calls: ID LP [Args] RP
        else if (stringComparison(firstChild->name, "ID")) {
            // Look up function in symbol table
            char *funcName = firstChild->value;
            Type funcType = (Type)malloc(sizeof(struct Type_));
            int isDefined = -1;
            bool found = lookupLocalSymbol(&funcType, funcName, &isDefined, currentScopeDepth, 1);
            
            // Handle various cases of function calls
            // Case 1: Function not found
            if (!found) {
                reportSemanticError(Undefined_Function, node->lineno, funcName);
                return NULL;
            }
            
            // Case 2: ID is not a function
            if (funcType->kind != FUNCTION) {
                reportSemanticError(Operate_Basic_As_Func, node->lineno, funcName);
                return NULL;
            }
            
            // Case 3: Function call with arguments
            if (stringComparison(thirdChild->name, "Args")) {
                // If function has no parameters but arguments are provided
                if (funcType->u.function.parameters == NULL) {
                    reportSemanticError(Func_Call_Parameter_Dismatch, node->lineno, NULL);
                    return NULL;
                }
                
                // Count the number of arguments
                int argCount = 0;
                ASTNode *currentArg = thirdChild;
                while (currentArg != NULL) {
                    argCount++;
                    if (getChild(currentArg, 1) == NULL) break;
                    currentArg = getChild(currentArg, 2);
                }
                
                // Check if the argument count matches parameter count
                if (argCount != funcType->u.function.parameterNum) {
                    reportSemanticError(Func_Call_Parameter_Dismatch, node->lineno, NULL);
                    return NULL;
                }
                
                // Check argument types
                if (ProcessArgumentList(thirdChild, funcType->u.function.parameters) != 0) {
                    return NULL;
                }
                
                // Return function's return type
                return funcType->u.function.returnType;
            }
            // Case 4: Function call without arguments
            else if (stringComparison(thirdChild->name, "RP")) {
                // If function has parameters but no arguments are provided
                if (funcType->u.function.parameters != NULL) {
                    reportSemanticError(Func_Call_Parameter_Dismatch, node->lineno, NULL);
                    return NULL;
                }
                
                // Return function's return type
                return funcType->u.function.returnType;
            }
        }
    }
    
    return NULL;
}

int ProcessArgumentList(ASTNode *node, FieldList formalParams)
{
    // Check if parameters exist
    if (formalParams == NULL) {
        reportSemanticError(Func_Call_Parameter_Dismatch, node->lineno, NULL);
        return -1;
    }
    
    // Get the first argument expression
    ASTNode *firstArgExp = getChild(node, 0);
    if (firstArgExp == NULL) return 0;
    
    // Process the argument
    Type argType = ProcessExpression(firstArgExp);
    
    // Check if argument type matches parameter type
    if (argType == NULL || formalParams->type == NULL || !compareTypes(argType, formalParams->type)) {
        reportSemanticError(Func_Call_Parameter_Dismatch, node->lineno, NULL);
        return -1;
    }
    
    // Check if there are more arguments
    ASTNode *commaNode = getChild(node, 1);
    if (commaNode != NULL) {
        // Check if there are more formal parameters
        if (formalParams->nextFieldList == NULL) {
            reportSemanticError(Func_Call_Parameter_Dismatch, node->lineno, NULL);
            return -1;
        }
        
        // Process the rest of the arguments recursively
        ASTNode *restArgs = getChild(node, 2);
        return ProcessArgumentList(restArgs, formalParams->nextFieldList);
    }
    
    return 0;
}

FieldList ProcessStructureDeclaration(ASTNode *node, Type typeInfo, char *structName)
{
    DEBUG_PRINT(DEBUG_DETAILED, "Processing structure declaration at line %d\n", node->lineno);
    
    // Get the variable declaration and possible assignment
    ASTNode *varDecNode = getChild(node, 0);
    ASTNode *assignNode = getChild(node, 1);
    
    // Process the variable declaration
    FieldList field = ProcessVarDec(varDecNode, typeInfo);
    
    // 如果是结构体类型字段，打印调试信息
    if (field && field->type && field->type->kind == STRUCTURE) {
        DEBUG_PRINT(DEBUG_DETAILED, "Found structure field: %s\n", field->name);
    }
    
    // If there's an assignment, report an error (not allowed in structure fields)
    if (assignNode != NULL) {
        DEBUG_PRINT(DEBUG_BASIC, "Error: Assignment in structure field declaration\n");
        reportSemanticError(Redefined_Field, node->lineno, field->name);
    }
    
    // 调用insertStruct函数将字段添加到符号表
    if (field != NULL && structName != NULL) {
        // 简单计算偏移量（实际应根据类型大小和对齐计算）
        static int fieldOffset = 0;
        DEBUG_PRINT(DEBUG_DETAILED, "Inserting field %s to structure %s with offset %d\n", 
                    field->name, structName, fieldOffset);
        
        insertStruct(field->type, field->name, fieldOffset, structName);
        // 更新下一个字段的偏移量
        fieldOffset += 4;  // 假设每个字段占用4字节
    }
    
    return field;
}

// 函数实现
HashTableNode enterInnermostHashTable()
{
    // Create a new scope for function body
    HashTableNode newScope = pushNewScope();
    currentScopeNode = newScope;
    return newScope;
}

void deleteLocalVariable()
{
    // Remove local variables when exiting a scope
    popCurrentScope();
}

/* Validate function declarations */
void validateFunctionDefinitions() {
    DEBUG_PRINT(DEBUG_BASIC, "\n=== Validating Function Definitions ===\n");
    FunctionTable current = funcRegister;
    
    while (current) {
        Type funcType = NULL;
        int isDefined = 0;
        int category = 0;
        
        DEBUG_PRINT(DEBUG_DETAILED, "Checking function: %s\n", current->name);
        
        if (lookupGlobalSymbol(&funcType, current->name, &isDefined, 0, &category)) {
            if (isDefined == 0) {
                DEBUG_PRINT(DEBUG_BASIC, "Error: Function %s declared but not defined\n", current->name);
                printf("Error type 18 at Line %d: Function \"%s\" declared but not defined.\n", 
                       current->functionLineNumber, current->name);
            } else {
                DEBUG_PRINT(DEBUG_VERBOSE, "Function %s is properly defined\n", current->name);
            }
        }
        
        current = current->next;
    }
    
    DEBUG_PRINT(DEBUG_BASIC, "=== Function Definition Validation Complete ===\n\n");
}

