#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantictool.h"

/* Global variables with renamed identifiers */
FunctionTable funcRegister = NULL;
HashTableNode currentScopeNode = NULL;
HashTableNode rootScopeNode = NULL;
HashTableNode_ symbolRegistry[TABLESIZE] = {NULL};
HashTableNode_ structRegistry[TABLESIZE] = {NULL};

/* Initialize a symbol scope table */
HashTableNode createSymbolScopeTable() {
    HashTableNode newScope = (HashTableNode)malloc(sizeof(HashTableNode_));
    if (!newScope) return NULL;
    
    newScope->symbolTableNode = NULL;
    newScope->nextHashTableNode = NULL;
    
    rootScopeNode = newScope;
    
    return newScope;
}

/* Create a new symbol table entry */
SymbolTableNode buildSymbolEntry(Type typeInfo, char* symbolIdentifier, int defineStatus, int scopeLevel) {
    SymbolTableNode entry = (SymbolTableNode)malloc(sizeof(SymbolTableNode_));
    if (!entry) return NULL;
    
    if (symbolIdentifier != NULL) {
        char* nameCopy = malloc((strlen(symbolIdentifier) + 1) * sizeof(char));
        if (nameCopy != NULL) {
            strcpy(nameCopy, symbolIdentifier);
            entry->name = nameCopy;
        } else {
            entry->name = NULL;
        }
    } else {
        entry->name = NULL;
    }
    
    entry->type = typeInfo;
    entry->depth = scopeLevel;
    entry->isDefined = defineStatus;
    entry->sameHashSymbolTableNode = NULL;
    entry->controlScopeSymbolTableNode = NULL;
    
    return entry;
}

/* Find a symbol in the scope hierarchy */
SymbolTableNode findSymbolInScope(char* symbolIdentifier, int scopeLevel) {
    if (symbolIdentifier == NULL) {
        printf("ERROR: findSymbolInScope接收到空的符号标识符\n");
        return NULL;
    }

    unsigned int hashIndex = hash_pjw(symbolIdentifier);
    printf("DEBUG: 查找符号: '%s', 作用域级别: %d, 哈希索引: %u\n", 
           symbolIdentifier, scopeLevel, hashIndex);
    
    SymbolTableNode current = symbolRegistry[hashIndex].symbolTableNode;
    SymbolTableNode bestMatch = NULL;
    
    int checkCount = 0;
    printf("DEBUG: 哈希索引 %u 中的所有符号:\n", hashIndex);
    SymbolTableNode debugCurrent = current;
    while (debugCurrent) {
        printf("  符号: '%s', 深度: %d, 种类: %d, 已定义: %d\n", 
               debugCurrent->name ? debugCurrent->name : "NULL", 
               debugCurrent->depth, 
               debugCurrent->kind, 
               debugCurrent->isDefined);
        debugCurrent = debugCurrent->sameHashSymbolTableNode;
    }
    
    while (current) {
        checkCount++;
        printf("DEBUG: [%d] 比较符号: '%s' 与 '%s', 深度: %d, 地址: %p\n", 
               checkCount, current->name ? current->name : "NULL", 
               symbolIdentifier, current->depth, (void*)current);
               
        if (current->name != NULL && stringComparison(current->name, symbolIdentifier) && scopeLevel >= current->depth) {
            bestMatch = current;
            printf("DEBUG: 找到匹配符号: '%s', 深度: %d\n", bestMatch->name, bestMatch->depth);
        }
        
        current = current->sameHashSymbolTableNode;
    }
    
    if (bestMatch) {
        printf("DEBUG: 返回最佳匹配符号: '%s', 深度: %d\n", bestMatch->name, bestMatch->depth);
    } else {
        printf("DEBUG: 未找到符号: '%s'\n", symbolIdentifier);
    }
    
    return bestMatch;
}

/* Register a symbol in the symbol table */
void registerSymbol(SymbolTableNode entry, HashTableNode scopeTable) {
    if (entry == NULL) {
        printf("ERROR: registerSymbol接收到空的entry参数\n");
        return;
    }
    
    if (scopeTable == NULL) {
        printf("ERROR: registerSymbol接收到空的scopeTable参数\n");
        return;
    }
    
    if (entry->name == NULL) {
        printf("ERROR: registerSymbol接收到空的符号名称\n");
        return;
    }
    
    printf("DEBUG: 注册符号: '%s', 种类: %d, 深度: %d\n", 
           entry->name, entry->kind, entry->depth);
    
    unsigned int hashIndex = hash_pjw(entry->name);
    printf("DEBUG: 符号哈希索引: %u\n", hashIndex);
    
    if (!scopeTable->symbolTableNode) {
        printf("DEBUG: 作用域表为空，添加为第一个符号\n");
        scopeTable->symbolTableNode = entry;
    } else {
        SymbolTableNode last = scopeTable->symbolTableNode;
        while (last->controlScopeSymbolTableNode) {
            last = last->controlScopeSymbolTableNode;
        }
        
        printf("DEBUG: 将符号添加到作用域链末尾\n");
        last->controlScopeSymbolTableNode = entry;
    }
    
    printf("DEBUG: 将符号添加到哈希表: 索引=%u\n", hashIndex);
    entry->sameHashSymbolTableNode = symbolRegistry[hashIndex].symbolTableNode;
    symbolRegistry[hashIndex].symbolTableNode = entry;
    
    printf("DEBUG: 符号'%s'注册完成\n", entry->name);
}

/* Create a new nested scope */
HashTableNode pushNewScope() {
    HashTableNode newScope = malloc(sizeof(HashTableNode_));
    if (!newScope) return NULL;
    
    newScope->nextHashTableNode = NULL;
    newScope->symbolTableNode = NULL;
    
    HashTableNode current = rootScopeNode;
    while (current && current->nextHashTableNode) {
        current = current->nextHashTableNode;
    }
    
    if (current) {
        current->nextHashTableNode = newScope;
    }
    
    return newScope;
}

/* Remove variables from innermost scope */
void popCurrentScope() {
    // HashTableNode parent = rootScopeNode;
    // HashTableNode target = parent;
    
    // while (target->nextHashTableNode) {
    //     parent = target;
    //     target = target->nextHashTableNode;
    // }
    
    // if (!parent) return;
    
    // while (target->symbolTableNode) {
    //     SymbolTableNode nodeToRemove = target->symbolTableNode;
    //     removeSymbolFromTable(nodeToRemove->name, nodeToRemove->depth, target);
    // }
    
    // parent->nextHashTableNode = NULL;
}

/* Remove a symbol from the table */
void removeSymbolFromTable(char* symbolIdentifier, int scopeLevel, HashTableNode targetScope) {
    if (!symbolIdentifier || !targetScope) {
        return;
    }

    unsigned int hashIndex = hash_pjw(symbolIdentifier);
    
    // Check if there are any symbols in this hash bucket
    if (!symbolRegistry[hashIndex].symbolTableNode) {
        return;
    }

    // Special handling for the first node in the hash chain
    SymbolTableNode firstNode = symbolRegistry[hashIndex].symbolTableNode;
    if (stringComparison(firstNode->name, symbolIdentifier) && firstNode->depth == scopeLevel) {
        symbolRegistry[hashIndex].symbolTableNode = firstNode->sameHashSymbolTableNode;
        
        // Also remove from scope chain if it's the first node
        if (targetScope->symbolTableNode == firstNode) {
            targetScope->symbolTableNode = firstNode->controlScopeSymbolTableNode;
        } else {
            // Find and remove from scope chain
            SymbolTableNode scopePrev = targetScope->symbolTableNode;
            while (scopePrev && scopePrev->controlScopeSymbolTableNode != firstNode) {
                scopePrev = scopePrev->controlScopeSymbolTableNode;
            }
            if (scopePrev) {
                scopePrev->controlScopeSymbolTableNode = firstNode->controlScopeSymbolTableNode;
            }
        }
        
        // Free the removed node
        if (firstNode->name) {
            free(firstNode->name);
        }
        free(firstNode);
        return;
    }

    // Search through the hash chain
    SymbolTableNode hashPrev = firstNode;
    SymbolTableNode hashCurrent = firstNode->sameHashSymbolTableNode;
    
    while (hashCurrent) {
        if (stringComparison(hashCurrent->name, symbolIdentifier) && hashCurrent->depth == scopeLevel) {
            // Remove from hash chain
            hashPrev->sameHashSymbolTableNode = hashCurrent->sameHashSymbolTableNode;
            
            // Remove from scope chain
            if (targetScope->symbolTableNode == hashCurrent) {
                targetScope->symbolTableNode = hashCurrent->controlScopeSymbolTableNode;
            } else {
                SymbolTableNode scopePrev = targetScope->symbolTableNode;
                while (scopePrev && scopePrev->controlScopeSymbolTableNode != hashCurrent) {
                    scopePrev = scopePrev->controlScopeSymbolTableNode;
                }
                if (scopePrev) {
                    scopePrev->controlScopeSymbolTableNode = hashCurrent->controlScopeSymbolTableNode;
                }
            }
            
            // Free the removed node
            if (hashCurrent->name) {
                free(hashCurrent->name);
            }
            free(hashCurrent);
            return;
        }
        hashPrev = hashCurrent;
        hashCurrent = hashCurrent->sameHashSymbolTableNode;
    }
}

/* Construct a new symbol table entry with extended information */
SymbolTableNode constructSymbolEntry(Type typeInfo, char* symbolIdentifier, int category, bool defineStatus, int scopeLevel) {
    SymbolTableNode entry = (SymbolTableNode)malloc(sizeof(SymbolTableNode_));
    if (!entry) return NULL;
    
    printf("DEBUG: 创建符号表项: 符号='%s', 种类=%d, 作用域深度=%d\n", 
           symbolIdentifier ? symbolIdentifier : "NULL", category, scopeLevel);
    
    if (symbolIdentifier != NULL) {
        char* nameCopy = (char*)malloc((strlen(symbolIdentifier) + 1) * sizeof(char));
        if (nameCopy != NULL) {
            strcpy(nameCopy, symbolIdentifier);
            entry->name = nameCopy;
            printf("DEBUG: 成功复制符号名称: '%s'\n", entry->name);
        } else {
            entry->name = NULL;
            printf("ERROR: 无法为符号名称分配内存\n");
        }
    } else {
        entry->name = NULL;
        printf("WARNING: 符号标识符为NULL\n");
    }
    
    entry->type = typeInfo;
    entry->kind = category;
    entry->isDefined = defineStatus;
    entry->depth = scopeLevel;
    entry->sameHashSymbolTableNode = NULL;
    entry->controlScopeSymbolTableNode = NULL;
    
    return entry;
}

/* insertStruct 插入结构体节点 */
int insertStruct(Type type, char *name, int offset, char *structName)

{
    printf("DEBUG: 插入结构体节点: '%s', 偏移量=%d, 结构体名称=%s\n", 
           name,  offset, structName);
    
    SymbolTableNode insertSymbolTableNode = buildSymbolEntry(type, name, 1, 0);
    insertSymbolTableNode->kind = 0;
    insertSymbolTableNode->offset = offset;
    insertSymbolTableNode->structName = structName;
    registerSymbol(insertSymbolTableNode, symbolRegistry);
    if (structRegistry[hash_pjw(name)].symbolTableNode == NULL)
    {
        SymbolTableNode currentSymbolTableNode = (SymbolTableNode)malloc(sizeof(struct SymbolTableNode_));
        currentSymbolTableNode->type = type;
        currentSymbolTableNode->offset = offset;
        currentSymbolTableNode->structName = structName;
        
        // 为name分配内存
        currentSymbolTableNode->name = (char *)malloc(strlen(name) + 1);
        if (currentSymbolTableNode->name != NULL) {
            strcpy(currentSymbolTableNode->name, name);
        }
        
        currentSymbolTableNode->sameHashSymbolTableNode = NULL;
        structRegistry[hash_pjw(name)].symbolTableNode = currentSymbolTableNode;
    }
    else
    {
        SymbolTableNode head = structRegistry[hash_pjw(name)].symbolTableNode;
        SymbolTableNode iterator = head;
        while (iterator->sameHashSymbolTableNode != NULL)
        {
            if (strcmp(head->name, name) == 0)
            {
                return 1;
            }
            iterator = iterator->sameHashSymbolTableNode;  // 修复循环逻辑错误
        }
        // 再检查最后一个节点
        if (strcmp(iterator->name, name) == 0) {
            return 1;
        }
        
        SymbolTableNode currentSymbolTableNode = malloc(sizeof(struct SymbolTableNode_));
        currentSymbolTableNode->offset = offset;
        currentSymbolTableNode->type = type;
        currentSymbolTableNode->sameHashSymbolTableNode = head;
        
        // 为name分配内存
        currentSymbolTableNode->name = (char *)malloc(strlen(name) + 1);
        if (currentSymbolTableNode->name == NULL) {
            free(currentSymbolTableNode);
            return -1;
        }
        strcpy(currentSymbolTableNode->name, name);
        
        // 为structName分配内存，确保它不是空指针
        if (structName != NULL) {
            currentSymbolTableNode->structName = (char *)malloc(strlen(structName) + 1);
            if (currentSymbolTableNode->structName != NULL) {
                strcpy(currentSymbolTableNode->structName, structName);
            }
        } else {
            currentSymbolTableNode->structName = NULL;
        }
        
        structRegistry[hash_pjw(name)].symbolTableNode = currentSymbolTableNode;
    }
    return 0;
}
/* Track function declarations for later validation */
void trackFunctionDeclaration(char* funcName, int linePosition) {
    if (!funcRegister) {
        funcRegister = (FunctionTable)malloc(sizeof(struct FunctionTable_));
        if (!funcRegister) return;
        
        funcRegister->name = funcName;
        funcRegister->functionLineNumber = linePosition;
        funcRegister->next = NULL;
        return;
    }
    
    FunctionTable current = funcRegister;
    while (current->next) {
        current = current->next;
    }
    
    FunctionTable newFunc = (FunctionTable)malloc(sizeof(struct FunctionTable_));
    if (!newFunc) return;
    
    newFunc->name = funcName;
    newFunc->functionLineNumber = linePosition;
    newFunc->next = NULL;
    
    current->next = newFunc;
}

/* Add a structure type to registry */
int addStructType(Type structTypeInfo, char* structIdentifier) {
    printf("DEBUG: 添加结构体类型: '%s'\n", structIdentifier);
    unsigned int hashIndex = hash_pjw(structIdentifier);
    
    if (!structRegistry[hashIndex].symbolTableNode) {
        SymbolTableNode entry = (SymbolTableNode)malloc(sizeof(SymbolTableNode_));
        if (!entry) return -1;
        
        entry->type = structTypeInfo;
        
        // 分配内存来存储名称
        entry->name = (char *)malloc(strlen(structIdentifier) + 1);
        if (!entry->name) {
            free(entry);
            return -1;
        }
        strcpy(entry->name, structIdentifier);
        
        entry->sameHashSymbolTableNode = NULL;
        
        structRegistry[hashIndex].symbolTableNode = entry;
    } else {
        SymbolTableNode firstNode = structRegistry[hashIndex].symbolTableNode;
        
        SymbolTableNode entry = (SymbolTableNode)malloc(sizeof(SymbolTableNode_));
        if (!entry) return -1;
        
        entry->type = structTypeInfo;
        
        // 分配内存来存储名称
        entry->name = (char *)malloc(strlen(structIdentifier) + 1);
        if (!entry->name) {
            free(entry);
            return -1;
        }
        strcpy(entry->name, structIdentifier);
        
        entry->sameHashSymbolTableNode = firstNode;
        
        structRegistry[hashIndex].symbolTableNode = entry;
    }
    
    return 0;
}

/* Find structure by name */
SymbolTableNode findStructByName(char* structIdentifier) {
    unsigned int hashIndex = hash_pjw(structIdentifier);
    
    if (!structRegistry[hashIndex].symbolTableNode) {
        return NULL;
    }
    
    SymbolTableNode current = structRegistry[hashIndex].symbolTableNode;
    SymbolTableNode result = NULL;
    
    while (current) {
        if (strcmp(current->name, structIdentifier) == 0) {
            result = current;
            break;
        }
        
        current = current->sameHashSymbolTableNode;
    }
    
    return result;
}

/* Look up a local symbol */
bool lookupLocalSymbol(Type* typeResult, char* symbolIdentifier, int* defineStatus, int scopeLevel, int visibilityMode) {
    unsigned int hashIndex = hash_pjw(symbolIdentifier);
    
    SymbolTableNode current = symbolRegistry[hashIndex].symbolTableNode;
    
    if (!current) return false;
    
    while (current) {
        bool nameMatch = stringComparison(current->name, symbolIdentifier);
        bool depthMatch = false;
        
        if (visibilityMode == 0) {
            depthMatch = (scopeLevel == current->depth);
        } else if (visibilityMode == 1) {
            depthMatch = (scopeLevel >= current->depth);
        }
        
        if (nameMatch && depthMatch) {
            *typeResult = current->type;
            *defineStatus = current->isDefined;
            return true;
        }
        
        current = current->sameHashSymbolTableNode;
    }
    
    return false;
}

/* Look up symbol in global scope */
bool lookupGlobalSymbol(Type* typeResult, char* symbolIdentifier, int* defineStatus, int scopeLevel, int* category) {
    unsigned int hashIndex = hash_pjw(symbolIdentifier);
    
    SymbolTableNode current = symbolRegistry[hashIndex].symbolTableNode;
    
    if (!current) return false;
    
    while (current) {
        if (stringComparison(current->name, symbolIdentifier) && scopeLevel >= current->depth) {
            *typeResult = current->type;
            *defineStatus = current->isDefined;
            *category = current->kind;
            return true;
        }
        
        current = current->sameHashSymbolTableNode;
    }
    
    return false;
}

/* Check if structure exists */
bool checkStructExists(Type* typeResult, char* structIdentifier) {
    unsigned int hashIndex = hash_pjw(structIdentifier);
    
    if (!structRegistry[hashIndex].symbolTableNode) {
        return false;
    }
    
    SymbolTableNode current = structRegistry[hashIndex].symbolTableNode;
    
    while (current) {
        if (stringComparison(current->name, structIdentifier)) {
            *typeResult = current->type;
            return true;
        }
        
        current = current->sameHashSymbolTableNode;
        if (!current) break;
    }
    
    return false;
}

/* Report semantic errors with appropriate messages */
void reportSemanticError(enum SemanticError errorCode, int linePosition, char* msgDetails) {
    printf("Error type %d at Line %d: ", errorCode + 1, linePosition);
    
    switch (errorCode) {
        case Undefined_Variable:
            printf("Undefined variable \"%s\".\n", msgDetails);
            break;
        case Undefined_Function:
            printf("Undefined function \"%s\".\n", msgDetails);
            break;
        case Redefined_Variable_Name:
            printf("Redefined variable \"%s\".\n", msgDetails);
            break;
        case Redefined_Function:
            printf("Redefined function \"%s\".\n", msgDetails);
            break;
        case AssignOP_Type_Dismatch:
            printf("Type mismatched for assignment.\n");
            break;
        case Leftside_Rvalue_Error:
            printf("The left-hand side of an assignment must be a variable.\n");
            break;
        case Operand_Type_Dismatch:
            printf("Type mismatched for operands.\n");
            break;
        case Return_Type_Dismatch:
            printf("Type mismatched for return.\n");
            break;
        case Func_Call_Parameter_Dismatch:
            printf("Function \"%s\" is not applicable for arguments.\n", msgDetails);
            break;
        case Operate_Others_As_Array:
            printf("\"%s\" is not an array.\n", msgDetails);
            break;
        case Operate_Basic_As_Func:
            printf("\"%s\" is not a function.\n", msgDetails);
            break;
        case Array_Float_Index:
            printf("Array index must be an integer.\n");
            break;
        case Operate_Others_As_Struct:
            printf("\"%s\" is not a structure.\n", msgDetails);
            break;
        case Undefined_Field:
            printf("Non-existent field \"%s\".\n", msgDetails);
            break;
        case Redefined_Field:
            printf("Redefined field \"%s\".\n", msgDetails);
            break;
        case Redefined_Field_Name:
            printf("Redefined structure \"%s\".\n", msgDetails);
            break;
        case Undefined_Struct:
            printf("Undefined structure \"%s\".\n", msgDetails);
            break;
        case Undefined_Function_But_Declaration:
            printf("Undefined function \"%s\".\n", msgDetails);
            break;
        case Conflict_Decordef_Funcion:
            printf("Conflicting types for \"%s\".\n", msgDetails);
            break;
        default:
            printf("Unknown error.\n");
    }
}

/* Compare two types for compatibility */
bool compareTypes(Type type1, Type type2) {
    if (!type1 || !type2) return false;
    
    if (type1->kind != type2->kind) return false;
    
    switch (type1->kind) {
        case BASIC:
            return type1->u.basic == type2->u.basic;
            
        case ARRAY:
            return compareTypes(type1->u.array.element, type2->u.array.element);
            
        case STRUCTURE: {
            // 如果两个结构体都有名字且名字相同，直接返回true
            if (type1->u.structure.name && type2->u.structure.name &&
                stringComparison(type1->u.structure.name, type2->u.structure.name)) {
                return true;
            }
            
            // 否则比较结构体的字段
            FieldList fields1 = type1->u.structure.structures;
            FieldList fields2 = type2->u.structure.structures;
            
            // 比较每个字段的类型
            while (fields1 && fields2) {
                if (!compareTypes(fields1->type, fields2->type)) {
                    return false;
                }
                fields1 = fields1->nextFieldList;
                fields2 = fields2->nextFieldList;
            }
            
            // 确保两个结构体有相同数量的字段
            return fields1 == NULL && fields2 == NULL;
        }
            
        case FUNCTION:
            if (!compareTypes(type1->u.function.returnType, type2->u.function.returnType)) {
                return false;
            }
            
            if (type1->u.function.parameterNum != type2->u.function.parameterNum) {
                return false;
            }
            
            FieldList params1 = type1->u.function.parameters;
            FieldList params2 = type2->u.function.parameters;
            
            while (params1 && params2) {
                if (!compareTypes(params1->type, params2->type)) {
                    return false;
                }
                
                params1 = params1->nextFieldList;
                params2 = params2->nextFieldList;
            }
            
            return params1 == NULL && params2 == NULL;
            
        default:
            return false;
    }
}

/* Compare array types with size checking */
bool compareArrayTypes(Type arrayType1, Type arrayType2) {
    if (!arrayType1 || !arrayType2) return false;
    
    if (arrayType1->kind != ARRAY || arrayType2->kind != ARRAY) {
        return false;
    }
    
    if (arrayType1->u.array.size != arrayType2->u.array.size) {
        return false;
    }
    
    return compareTypes(arrayType1->u.array.element, arrayType2->u.array.element);
} 