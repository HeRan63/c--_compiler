#include "intermediate.h"

/* 全局调试级别变量初始化 */
int IR_DEBUG_LEVEL = IR_DEBUG_NONE;  // 默认不输出调试信息

char *ir_invert_relop(char *relop);
//开始中间代码生成
/* ir_translate_program Program翻译 */
void ir_translate_program(ASTNode *root, FILE *file)
{
    // 合法性检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "程序的AST根节点为空，无法生成中间代码\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_INFO, "开始生成中间代码 (行号: %d)\n", root->lineno);
    
    // 初始化中间代码双向链表结构
    IR_DEBUG(IR_DEBUG_VERBOSE, "初始化中间代码存储结构\n");
    interCodeListHead = (InterCodes)malloc(sizeof(struct InterCodes_));
    if (!interCodeListHead) {
        IR_DEBUG(IR_DEBUG_ERROR, "内存分配失败，无法创建中间代码链表\n");
        return;
    }
    
    // 设置链表初始状态
    interCodeListHead->next = NULL;
    interCodeListHead->prev = NULL;
    interCodeListTail = interCodeListHead;
    
    // 获取并处理外部定义列表
    ASTNode *externalDefinitions = getChild(root, 0);
    if (!externalDefinitions) {
        IR_DEBUG(IR_DEBUG_ERROR, "程序缺少外部定义列表\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理外部定义列表\n");
    ir_translate_ext_def_list(externalDefinitions);
    
    // 输出中间代码到文件
    IR_DEBUG(IR_DEBUG_INFO, "中间代码生成完成，准备写入文件\n");
    //ir_write_codes(file);
}

/* ir_translate_ext_def_list  ExtDefList翻译 */
void ir_translate_ext_def_list(ASTNode *root)
{
    /*
    ExfDefList -> ExfDef ExfDefList
    | (empty)
    */
    // 检查输入有效性
    if (!root) {
        IR_DEBUG(IR_DEBUG_INFO, "外部定义列表为空，翻译结束\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理外部定义列表节点 (行号: %d)\n", root->lineno);
    
    // 提取当前外部定义和后续列表
    ASTNode *currentExtDef = getChild(root, 0);
    ASTNode *nextExtDefList = getChild(root, 1);
    
    // 处理当前外部定义
    if (currentExtDef) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "翻译当前外部定义\n");
        ir_translate_ext_def(currentExtDef);
        
        // 处理后续外部定义
        if (nextExtDefList) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "继续处理剩余外部定义\n");
            ir_translate_ext_def_list(nextExtDefList);
        } else {
            IR_DEBUG(IR_DEBUG_VERBOSE, "外部定义列表处理完毕\n");
        }
    } else {
        IR_DEBUG(IR_DEBUG_INFO, "外部定义节点为空\n");
    }
}

/* ir_translate_ext_def ExtDef翻译 */
void ir_translate_ext_def(ASTNode *root)
{
    /*
    ExtDef -> Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    | Specifier FunDec SEMI
    */
    // 检查输入有效性
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "外部定义节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理外部定义节点 (行号: %d)\n", root->lineno);
    
    // 提取子节点
    ASTNode *specifierNode = getChild(root, 0);
    ASTNode *secondNode = getChild(root, 1);
    ASTNode *thirdNode = getChild(root, 2);
    
    if (!specifierNode || !secondNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "外部定义节点结构不完整\n");
        return;
    }
    
    // 根据语法规则分情况处理
    const char *secondNodeName = secondNode->name;
    
    // 处理变量声明列表
    if (stringComparison(secondNodeName, "ExtDecList")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理全局变量声明\n");
        ir_translate_ext_dec_list(secondNode);
    } 
    // 处理函数定义
    else if (stringComparison(secondNodeName, "FunDec")) {
        if (thirdNode && stringComparison(thirdNode->name, "CompSt")) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理函数定义(带函数体)\n");
            ir_translate_fun_dec(secondNode);
            ir_translate_comp_st(thirdNode);
        } else {
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理函数声明(无函数体)\n");
            // 函数声明不生成中间代码
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理类型定义或其他声明\n");
        // 纯类型定义或其他情况，不需要生成代码
    }
}

/* ir_translate_ext_dec_list ExtDecList翻译 */
void ir_translate_ext_dec_list(ASTNode *root)
{
    /*
    ExtDecList -> VarDec
    | VarDec COMMA ExtDecList
    */
    // 检查输入有效性
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "外部声明列表节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理外部声明列表节点 (行号: %d)\n", root->lineno);
    
    // 提取子节点
    ASTNode *varDecNode = getChild(root, 0);
    ASTNode *commaNode = getChild(root, 1);
    ASTNode *nextExtDecListNode = getChild(root, 2);
    
    // 处理变量声明
    if (varDecNode) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "翻译变量声明\n");
        Operand varOperand = ir_translate_var_dec(varDecNode);
        
        if (!varOperand) {
            IR_DEBUG(IR_DEBUG_ERROR, "变量声明翻译失败\n");
        }
        
        // 处理后续声明
        if (commaNode && nextExtDecListNode) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "继续处理后续变量声明\n");
            ir_translate_ext_dec_list(nextExtDecListNode);
        } else {
            IR_DEBUG(IR_DEBUG_VERBOSE, "变量声明列表处理完毕\n");
        }
    } else {
        IR_DEBUG(IR_DEBUG_ERROR, "变量声明节点缺失\n");
    }
}

/* 变量声明翻译 - 返回对应的操作数 */
Operand ir_translate_var_dec(ASTNode *root)
{
    /*
    VarDec -> ID
    | VarDec LB INT RB
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "变量声明节点为空\n");
        return NULL;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理变量声明 (行号: %d)\n", root->lineno);
    
    // 获取第一个子节点
    ASTNode *firstChild = getChild(root, 0);
    if (!firstChild) {
        IR_DEBUG(IR_DEBUG_ERROR, "变量声明缺少子节点\n");
        return NULL;
    }
    
    // 返回的操作数
    Operand resultOperand = NULL;
    
    // 处理简单变量声明 (VarDec -> ID)
    if (stringComparison(firstChild->name, "ID")) {
        const char *varName = firstChild->value;
        IR_DEBUG(IR_DEBUG_INFO, "处理ID类型的变量声明: %s\n", varName);
        
        // 在符号表中查找变量
        SymbolTableNode symbolEntry = findSymbolInScope(varName, __INT_MAX__);
        if (!symbolEntry) {
            IR_DEBUG(IR_DEBUG_ERROR, "符号表中未找到变量: %s\n", varName);
            return NULL;
        }
        
        // 创建变量操作数
        resultOperand = ir_create_operand(VARIABLE_OP, VAL, varName);
        if (!resultOperand) {
            IR_DEBUG(IR_DEBUG_ERROR, "为变量 %s 创建操作数失败\n", varName);
            return NULL;
        }
        
        // 更新符号表中的地址类型和变量编号
        symbolEntry->isAddress = resultOperand->type;
        symbolEntry->var_no = resultOperand->var_no;
        
        // 检查是否需要为非标准大小类型分配空间
        int typeSize = ir_calc_type_size(symbolEntry->type);
        if (typeSize != 4) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "为非标准大小类型(%d字节)分配空间\n", typeSize);
            Operand sizeOperand = ir_create_operand(CONSTANT_OP, VAL, typeSize);
            ir_generate_code(DEC_InterCode, resultOperand, sizeOperand);
        }
    }
    // 处理数组声明 (VarDec -> VarDec LB INT RB)
    else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理数组类型的变量声明\n");
        
        // 向下查找ID节点
        ASTNode *idNode = firstChild;
        while (idNode && !stringComparison(idNode->name, "ID")) {
            idNode = idNode->firstChild;
        }
        
        if (!idNode) {
            IR_DEBUG(IR_DEBUG_ERROR, "在数组声明中找不到ID节点\n");
            return NULL;
        }
        
        const char *arrayName = idNode->value;
        IR_DEBUG(IR_DEBUG_INFO, "处理数组声明: %s\n", arrayName);
        
        // 在符号表中查找数组
        SymbolTableNode symbolEntry = findSymbolInScope(arrayName, __INT_MAX__);
        if (!symbolEntry) {
            IR_DEBUG(IR_DEBUG_ERROR, "符号表中未找到数组: %s\n", arrayName);
            return NULL;
        }
        
        // 创建数组变量操作数
        resultOperand = ir_create_operand(VARIABLE_OP, VAL, arrayName);
        if (!resultOperand) {
            IR_DEBUG(IR_DEBUG_ERROR, "为数组 %s 创建操作数失败\n", arrayName);
            return NULL;
        }
        
        // 更新符号表中的地址类型和变量编号
        symbolEntry->isAddress = resultOperand->type;
        symbolEntry->var_no = resultOperand->var_no;
        
        // 为数组分配空间
        int arraySize = ir_calc_type_size(symbolEntry->type);
        IR_DEBUG(IR_DEBUG_VERBOSE, "为数组分配空间: %d字节\n", arraySize);
        Operand sizeOperand = ir_create_operand(CONSTANT_OP, VAL, arraySize);
        ir_generate_code(DEC_InterCode, resultOperand, sizeOperand);
    }
    
    return resultOperand;
}

/* 函数声明翻译 */
void ir_translate_fun_dec(ASTNode *root)
{
    /*
    FunDec -> ID LP VarList RP
    | ID LP RP
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "函数声明节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理函数声明 (行号: %d)\n", root->lineno);
    
    // 获取函数名节点
    ASTNode *idNode = getChild(root, 0);
    if (!idNode || !stringComparison(idNode->name, "ID")) {
        IR_DEBUG(IR_DEBUG_ERROR, "函数声明缺少有效的函数名\n");
        return;
    }
    
    const char *funcName = idNode->value;
    IR_DEBUG(IR_DEBUG_INFO, "处理函数: %s\n", funcName);
    
    // 创建函数操作数并生成函数定义中间代码
    Operand functionOperand = ir_create_operand(FUNCTION_OP, VAL, funcName);
    if (!functionOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "为函数 %s 创建操作数失败\n", funcName);
        return;
    }
    
    ir_generate_code(FUNC_InterCode, functionOperand);
    
    // 在符号表中查找函数信息
    SymbolTableNode functionSymbol = findSymbolInScope(funcName, __INT_MAX__);
    if (!functionSymbol) {
        IR_DEBUG(IR_DEBUG_ERROR, "符号表中未找到函数: %s\n", funcName);
        return;
    }
    
    // 处理函数参数
    int paramCount = functionSymbol->type->u.function.parameterNum;
    if (paramCount > 0) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理函数 %s 的 %d 个参数\n", funcName, paramCount);
        
        // 获取参数列表
        FieldList currentParam = functionSymbol->type->u.function.parameters;
        int paramIndex = 0;
        
        // 遍历参数列表
        while (currentParam) {
            paramIndex++;
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理第 %d 个参数: %s\n", paramIndex, currentParam->name);
            
            // 创建参数操作数
            Operand paramOperand = NULL;
            
            // 根据参数类型创建不同的操作数
            if (currentParam->type->kind == ARRAY || currentParam->type->kind == STRUCTURE) {
                IR_DEBUG(IR_DEBUG_VERBOSE, "参数 %s 是复杂类型 (数组或结构体)\n", currentParam->name);
                paramOperand = ir_create_operand(VARIABLE_OP, ADDRESS, currentParam->name);
            } else {
                IR_DEBUG(IR_DEBUG_VERBOSE, "参数 %s 是基本类型\n", currentParam->name);
                paramOperand = ir_create_operand(VARIABLE_OP, VAL, currentParam->name);
            }
            
            if (!paramOperand) {
                IR_DEBUG(IR_DEBUG_ERROR, "为参数 %s 创建操作数失败\n", currentParam->name);
                currentParam = currentParam->nextFieldList;
                continue;
            }
            
            // 更新参数在符号表中的信息
            SymbolTableNode paramSymbol = findSymbolInScope(currentParam->name, __INT_MAX__);
            if (paramSymbol) {
                paramSymbol->var_no = paramOperand->var_no;
                paramSymbol->isAddress = paramOperand->type;
                
                // 确保生成PARAM指令时类型是VARIABLE_OP
                paramOperand->type = VARIABLE_OP;
                
                // 生成参数中间代码
                ir_generate_code(PARAM_InterCode, paramOperand);
            } else {
                IR_DEBUG(IR_DEBUG_ERROR, "符号表中未找到参数: %s\n", currentParam->name);
            }
            
            // 移动到下一个参数
            currentParam = currentParam->nextFieldList;
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "函数 %s 没有参数\n", funcName);
    }
}

/* 参数列表翻译 */
void ir_translate_var_list(ASTNode *root)
{
    /*
    VarList -> ParamDec COMMA VarList
    | ParamDec
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "参数列表节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理参数列表 (行号: %d)\n", root->lineno);
    
    // 提取各个子节点
    ASTNode *paramDecNode = getChild(root, 0);
    ASTNode *commaNode = getChild(root, 1);
    ASTNode *nextVarListNode = getChild(root, 2);
    
    // 处理第一个参数声明
    if (paramDecNode) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理参数声明\n");
        ir_translate_param_dec(paramDecNode);
    } else {
        IR_DEBUG(IR_DEBUG_ERROR, "参数列表缺少参数声明节点\n");
        return;
    }
    
    // 递归处理后续参数列表
    if (commaNode && nextVarListNode) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "发现更多参数，继续处理\n");
        ir_translate_var_list(nextVarListNode);
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "参数列表处理完毕\n");
    }
}

/* 参数声明翻译 */
void ir_translate_param_dec(ASTNode *root)
{
    /*
    ParamDec -> Specifier VarDec
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "参数声明节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理参数声明 (行号: %d)\n", root->lineno);
    
    // 提取各个子节点
    ASTNode *specifierNode = getChild(root, 0);
    ASTNode *varDecNode = getChild(root, 1);
    
    // 验证节点有效性
    if (!specifierNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "参数声明缺少类型说明符\n");
        return;
    }
    
    if (!varDecNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "参数声明缺少变量声明\n");
        return;
    }
    
    // 处理变量声明部分
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理参数的变量声明部分\n");
    Operand paramOperand = ir_translate_var_dec(varDecNode);
    
    if (!paramOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "参数变量声明处理失败\n");
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "参数声明处理完成\n");
    }
}

/* 复合语句翻译 */
void ir_translate_comp_st(ASTNode *root)
{
    /*
    CompSt -> LC DefList StmtList RC
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "复合语句节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理复合语句 (行号: %d)\n", root->lineno);
    
    // 提取复合语句内的各个部分
    ASTNode *lcNode = getChild(root, 0);         // 左大括号
    ASTNode *secondNode = getChild(root, 1);     // DefList 或 StmtList
    ASTNode *thirdNode = getChild(root, 2);      // StmtList 或 RC
    ASTNode *rcNode = getChild(root, 3);         // 右大括号或NULL
    
    if (!lcNode || !stringComparison(lcNode->name, "LC")) {
        IR_DEBUG(IR_DEBUG_ERROR, "复合语句缺少左大括号\n");
        return;
    }
    
    // 检查语法结构并相应处理
    if (secondNode) {
        // 判断第二个节点是否为DefList
        if (stringComparison(secondNode->name, "DefList")) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理复合语句中的局部变量定义\n");
            ir_translate_def_list(secondNode);
            
            // DefList后面应该是StmtList
            if (thirdNode && stringComparison(thirdNode->name, "StmtList")) {
                IR_DEBUG(IR_DEBUG_VERBOSE, "处理复合语句中的语句列表\n");
                ir_translate_stmt_list(thirdNode);
            } else if (thirdNode) {
                IR_DEBUG(IR_DEBUG_ERROR, "复合语句结构异常: DefList后应为StmtList，实际为%s\n", 
                         thirdNode ? thirdNode->name : "NULL");
            }
        }
        // 第二个节点是StmtList (没有局部变量定义的情况)
        else if (stringComparison(secondNode->name, "StmtList")) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理复合语句中的语句列表(无局部变量)\n");
            ir_translate_stmt_list(secondNode);
        } 
        else {
            IR_DEBUG(IR_DEBUG_ERROR, "复合语句结构异常: 预期DefList或StmtList，实际为%s\n", 
                     secondNode->name);
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "复合语句为空块\n");
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "复合语句处理完成\n");
}

/* 语句列表翻译处理 */
void ir_translate_stmt_list(ASTNode *root)
{
    /*
    StmtList -> Stmt StmtList
    | (empty)
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_INFO, "语句列表为空，结束处理\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理语句列表 (行号: %d)\n", root->lineno);
    
    // 获取第一个语句和后续语句列表
    ASTNode *currentStmt = getChild(root, 0);
    ASTNode *remainingStmts = getChild(root, 1);
    
    // 处理当前语句
    if (currentStmt) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理当前语句\n");
        ir_translate_stmt(currentStmt);
        
        // 处理后续语句
        if (remainingStmts) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "继续处理后续语句列表\n");
            ir_translate_stmt_list(remainingStmts);
        } else {
            IR_DEBUG(IR_DEBUG_VERBOSE, "无后续语句，语句列表处理完毕\n");
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "语句列表中无语句\n");
    }
}

/* 单个语句翻译处理 */
void ir_translate_stmt(ASTNode *root)
{
    /*
    Stmt -> Exp SEMI                  表达式语句
    | CompSt                         复合语句
    | RETURN Exp SEMI                返回语句
    | IF LP Exp RP Stmt              if语句
    | IF LP Exp RP Stmt ELSE Stmt    if-else语句
    | WHILE LP Exp RP Stmt           while循环语句
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "语句节点为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理语句 (行号: %d)\n", root->lineno);
    
    // 获取第一个子节点，确定语句类型
    ASTNode *firstNode = getChild(root, 0);
    if (!firstNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "语句缺少首个子节点\n");
        return;
    }
    
    // 根据语句类型进行处理
    const char *stmtType = firstNode->name;
    IR_DEBUG(IR_DEBUG_VERBOSE, "语句类型：%s\n", stmtType);
    
    // 表达式语句: Stmt -> Exp SEMI
    if (stringComparison(stmtType, "Exp")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理表达式语句\n");
        Operand expResult = ir_translate_exp(firstNode);
        if (!expResult) {
            IR_DEBUG(IR_DEBUG_ERROR, "表达式求值失败\n");
        }
    }
    // 复合语句: Stmt -> CompSt
    else if (stringComparison(stmtType, "CompSt")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理复合语句\n");
        ir_translate_comp_st(firstNode);
    }
    // 返回语句: Stmt -> RETURN Exp SEMI
    else if (stringComparison(stmtType, "RETURN")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理返回语句\n");
        
        // 获取返回表达式
        ASTNode *returnExp = getChild(root, 1);
        if (!returnExp) {
            IR_DEBUG(IR_DEBUG_ERROR, "返回语句缺少表达式\n");
            return;
        }
        
        // 计算返回值并生成返回中间代码
        Operand returnValue = ir_translate_exp(returnExp);
        if (!returnValue) {
            IR_DEBUG(IR_DEBUG_ERROR, "返回表达式计算失败\n");
            return;
        }
        
        IR_DEBUG(IR_DEBUG_VERBOSE, "生成RETURN中间代码\n");
        ir_generate_code(RETURN_InterCode, returnValue);
    }
    // while循环: Stmt -> WHILE LP Exp RP Stmt
    else if (stringComparison(stmtType, "WHILE")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理while循环\n");
        
        // 获取循环条件表达式和循环体
        ASTNode *condExp = getChild(root, 2);
        ASTNode *loopBody = getChild(root, 4);
        
        if (!condExp || !loopBody) {
            IR_DEBUG(IR_DEBUG_ERROR, "while循环结构不完整\n");
            return;
        }
        
        // 创建循环开始和结束标签
        Operand startLabel = ir_create_operand(LABEL_OP, VAL);
        Operand endLabel = ir_create_operand(LABEL_OP, VAL);
        
        // 生成循环起始标签
        IR_DEBUG(IR_DEBUG_VERBOSE, "生成循环起始标签\n");
        ir_generate_code(LABEL_InterCode, startLabel);
        
        // 处理条件表达式，为假时跳转到结束标签
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理循环条件\n");
        ir_translate_cond(condExp, NULL, endLabel);
        
        // 处理循环体
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理循环体\n");
        ir_translate_stmt(loopBody);
        
        // 循环结束返回到开始处
        IR_DEBUG(IR_DEBUG_VERBOSE, "生成循环跳转中间代码\n");
        ir_generate_code(GOTO_InterCode, startLabel);
        
        // 生成循环结束标签
        IR_DEBUG(IR_DEBUG_VERBOSE, "生成循环结束标签\n");
        ir_generate_code(LABEL_InterCode, endLabel);
    }
    // if和if-else语句
    else if (stringComparison(stmtType, "IF")) {
        // 获取条件表达式和第一个分支主体
        ASTNode *condExp = getChild(root, 2);
        ASTNode *thenStmt = getChild(root, 4);
        ASTNode *elseNode = getChild(root, 5);  // ELSE关键字或NULL
        
        if (!condExp || !thenStmt) {
            IR_DEBUG(IR_DEBUG_ERROR, "if语句结构不完整\n");
            return;
        }
        
        // 创建false分支标签
        Operand falseLabel = ir_create_operand(LABEL_OP, VAL);
        
        // 处理条件表达式
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理if条件表达式\n");
        ir_translate_cond(condExp, NULL, falseLabel);
        
        // 处理then分支
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理if的then分支\n");
        ir_translate_stmt(thenStmt);
        
        // 根据是否有else分支进行不同处理
        if (!elseNode) {
            // 简单if语句: IF LP Exp RP Stmt
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理简单if语句，生成条件为假时的标签\n");
            ir_generate_code(LABEL_InterCode, falseLabel);
        } else {
            // if-else语句: IF LP Exp RP Stmt ELSE Stmt
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理if-else语句\n");
            
            // 获取else分支语句
            ASTNode *elseStmt = getChild(root, 6);
            if (!elseStmt) {
                IR_DEBUG(IR_DEBUG_ERROR, "else分支语句缺失\n");
                ir_generate_code(LABEL_InterCode, falseLabel);
                return;
            }
            
            // 创建end标签，then分支执行完跳转到end
            Operand endLabel = ir_create_operand(LABEL_OP, VAL);
            IR_DEBUG(IR_DEBUG_VERBOSE, "生成if-else结束标签\n");
            ir_generate_code(GOTO_InterCode, endLabel);
            
            // 生成else分支的开始标签（即if条件为假时的标签）
            IR_DEBUG(IR_DEBUG_VERBOSE, "生成else分支开始标签\n");
            ir_generate_code(LABEL_InterCode, falseLabel);
            
            // 处理else分支
            IR_DEBUG(IR_DEBUG_VERBOSE, "处理else分支语句\n");
            ir_translate_stmt(elseStmt);
            
            // 生成整个if-else结束标签
            IR_DEBUG(IR_DEBUG_VERBOSE, "生成if-else整体结束标签\n");
            ir_generate_code(LABEL_InterCode, endLabel);
        }
    } else {
        IR_DEBUG(IR_DEBUG_ERROR, "未知语句类型: %s\n", stmtType);
    }
}

/* ir_translate_def_list DefList翻译 */
void ir_translate_def_list(ASTNode *root)
{
    /*
    DefList -> Def DefList
    | (empty)
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_INFO, "DefList为空，跳过处理\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理DefList节点（行号: %d）\n", root->lineno);
    
    // 获取当前定义和后续定义列表
    ASTNode *currentDef = getChild(root, 0);
    ASTNode *remainingDefs = getChild(root, 1);
    
    // 处理当前定义（如果存在）
    if (currentDef) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理当前Def节点\n");
        ir_translate_def(currentDef);
        
        // 递归处理剩余的定义列表
        if (remainingDefs) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "递归处理后续DefList\n");
            ir_translate_def_list(remainingDefs);
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "当前DefList节点的第一个子节点为空\n");
    }
}

/* ir_translate_def Def翻译 */
void ir_translate_def(ASTNode *root)
{
    /*
    Def -> Specifier DecList SEMI
    */
    // 检查输入有效性
    if (!root) {
        IR_DEBUG(IR_DEBUG_INFO, "Def节点为空，跳过处理\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理Def节点（行号: %d）\n", root->lineno);
    
    // 获取声明列表节点
    ASTNode *specifierNode = getChild(root, 0);
    ASTNode *declarationsNode = getChild(root, 1);
    ASTNode *semiNode = getChild(root, 2);
    
    // 验证语法结构
    if (!specifierNode || !declarationsNode || !semiNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "Def节点结构不完整 (Specifier: %p, DecList: %p, SEMI: %p)\n", 
                 specifierNode, declarationsNode, semiNode);
    }
    
    // 处理声明列表
    if (declarationsNode) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理声明列表(DecList)\n");
        ir_translate_dec_list(declarationsNode);
    } else {
        IR_DEBUG(IR_DEBUG_ERROR, "Def节点缺少DecList子节点\n");
    }
}

/* ir_translate_dec_list DecList翻译 */
void ir_translate_dec_list(ASTNode *root)
{
    /*
    DecList -> Dec
    | Dec COMMA DecList
    */
    // 检查输入有效性
    if (!root) {
        IR_DEBUG(IR_DEBUG_INFO, "DecList节点为空，跳过处理\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理DecList节点（行号: %d）\n", root->lineno);
    
    // 提取子节点
    ASTNode *declarationNode = getChild(root, 0);
    ASTNode *commaNode = getChild(root, 1);
    ASTNode *moreDeclarationsNode = getChild(root, 2);
    
    // 处理第一个声明
    if (declarationNode) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理声明(Dec)节点\n");
        ir_translate_dec(declarationNode);
    } else {
        IR_DEBUG(IR_DEBUG_ERROR, "DecList缺少Dec子节点\n");
        return;
    }
    
    // 检查是否有更多声明
    if (commaNode && moreDeclarationsNode) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "发现逗号，继续处理后续声明\n");
        ir_translate_dec_list(moreDeclarationsNode);
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "当前DecList处理完成，无后续声明\n");
    }
}

/* ir_translate_dec Dec翻译 */
void ir_translate_dec(ASTNode *root)
{
    /*
    Dec -> VarDec
    | VarDec ASSIGNOP Exp
    */
    // 检查输入有效性
    if (!root) {
        IR_DEBUG(IR_DEBUG_INFO, "Dec节点为空，跳过处理\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始处理Dec节点（行号: %d）\n", root->lineno);
    
    // 提取子节点
    ASTNode *varDecNode = getChild(root, 0);
    ASTNode *assignOpNode = getChild(root, 1);
    ASTNode *expressionNode = getChild(root, 2);
    
    // 检查变量声明节点
    if (!varDecNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "Dec节点缺少VarDec子节点\n");
        return;
    }
    
    // 区分简单声明和带赋值的声明
    if (!assignOpNode) {
        // 简单变量声明 Dec -> VarDec
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理简单变量声明\n");
        ir_translate_var_dec(varDecNode);
    } else {
        // 带赋值的变量声明 Dec -> VarDec ASSIGNOP Exp
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理带赋值的变量声明\n");
        
        // 处理左侧变量
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理声明的变量\n");
        Operand leftOperand = ir_translate_var_dec(varDecNode);
        
        // 处理右侧表达式
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理赋值表达式\n");
        Operand rightOperand = ir_translate_exp(expressionNode);
        
        // 生成赋值中间代码
        if (leftOperand && rightOperand) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "生成赋值中间代码\n");
            ir_generate_code(ASSIGN_InterCode, leftOperand, rightOperand);
        } else {
            IR_DEBUG(IR_DEBUG_ERROR, "赋值操作数无效: left=%p, right=%p\n", 
                     leftOperand, rightOperand);
        }
    }
}

/* 表达式翻译主函数 */
Operand ir_translate_exp(ASTNode *root)
{
    /*
    Exp -> Exp ASSIGNOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp RELOP Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID
    | INT
    | FLOAT
    */
    // 空节点检查
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "表达式节点为空\n");
        return NULL;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "开始翻译表达式 (行号: %d)\n", root->lineno);
    
    // 获取子节点
    ASTNode *firstChild = getChild(root, 0);
    if (!firstChild) {
        IR_DEBUG(IR_DEBUG_ERROR, "表达式节点缺少子节点\n");
        return NULL;
    }
    
    // 基于第一个子节点的类型进行分发
    const char *firstChildName = firstChild->name;
    
    // 第一个子节点是Exp的情况
    if (stringComparison(firstChildName, "Exp")) {
        ASTNode *operatorNode = getChild(root, 1);
        if (!operatorNode) {
            IR_DEBUG(IR_DEBUG_ERROR, "表达式缺少操作符\n");
            return NULL;
        }
        
        const char *opName = operatorNode->name;
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理二元表达式, 操作符: %s\n", opName);
        
        // 根据操作符类型分发到不同的处理函数
        if (stringComparison(opName, "ASSIGNOP")) {
            return ir_translate_assign_exp(root);
        } 
        else if (stringComparison(opName, "AND") || 
                 stringComparison(opName, "OR") || 
                 stringComparison(opName, "RELOP")) {
            return ir_translate_logical_exp(root);
        } 
        else if (stringComparison(opName, "PLUS") || 
                 stringComparison(opName, "MINUS") || 
                 stringComparison(opName, "STAR") || 
                 stringComparison(opName, "DIV")) {
            return ir_translate_arithmetic_exp(root);
        } 
        else if (stringComparison(opName, "LB")) {
            return ir_translate_array_access(root);
        } 
        else if (stringComparison(opName, "DOT")) {
            return ir_translate_field_access(root);
        } 
        else {
            IR_DEBUG(IR_DEBUG_ERROR, "未知的操作符类型: %s\n", opName);
            return NULL;
        }
    }
    // 括号表达式
    else if (stringComparison(firstChildName, "LP")) {
        return ir_translate_paren_exp(root);
    }
    // 负号表达式
    else if (stringComparison(firstChildName, "MINUS")) {
        return ir_translate_negative_exp(root);
    }
    // 非表达式
    else if (stringComparison(firstChildName, "NOT")) {
        return ir_translate_not_exp(root);
    }
    // 标识符相关表达式
    else if (stringComparison(firstChildName, "ID")) {
        ASTNode *secondChild = getChild(root, 1);
        
        // 区分变量引用和函数调用
        if (secondChild && stringComparison(secondChild->name, "LP")) {
            return ir_translate_call_exp(root);
        } else {
            return ir_translate_id_exp(root);
        }
    }
    // 常量表达式
    else if (stringComparison(firstChildName, "INT") || 
             stringComparison(firstChildName, "FLOAT")) {
        return ir_translate_constant_exp(root);
    }
    else {
        IR_DEBUG(IR_DEBUG_ERROR, "未知的表达式类型: %s\n", firstChildName);
        return NULL;
    }
}

/* 处理赋值表达式 */
Operand ir_translate_assign_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理赋值表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "赋值表达式节点为空\n");
        return NULL;
    }
    
    // 获取子节点
    ASTNode *leftExp = getChild(root, 0);
    ASTNode *assignOp = getChild(root, 1);
    ASTNode *rightExp = getChild(root, 2);
    
    if (!leftExp || !assignOp || !rightExp) {
        IR_DEBUG(IR_DEBUG_ERROR, "赋值表达式结构不完整\n");
        return NULL;
    }
    
    // 翻译左右表达式
    Operand leftOperand = ir_translate_exp(leftExp);
    Operand rightOperand = ir_translate_exp(rightExp);
    
    if (!leftOperand || !rightOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "赋值表达式操作数计算失败\n");
        return leftOperand; // 返回左操作数，即使为NULL
    }
    
    // 检查是否是数组间赋值
    int isArrayAssign = 0;
    if (leftOperand->varName && rightOperand->varName) {
        SymbolTableNode leftSymbol = findSymbolInScope(leftOperand->varName, __INT_MAX__);
        SymbolTableNode rightSymbol = findSymbolInScope(rightOperand->varName, __INT_MAX__);
        
        if (leftSymbol && rightSymbol && 
            leftSymbol->type->kind == ARRAY && 
            rightSymbol->type->kind == ARRAY && 
            leftOperand->type == VAL && 
            rightOperand->type == VAL) {
            isArrayAssign = 1;
        }
    }
    
    // 处理不同的赋值情况
    if (isArrayAssign) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理数组间赋值\n");
        return ir_translate_array_assign(leftOperand, rightOperand);
    } else {
        // 普通赋值
        if (leftOperand && rightOperand) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "生成普通赋值中间代码\n");
            ir_generate_code(ASSIGN_InterCode, leftOperand, rightOperand);
        }
        return leftOperand;
    }
}

/* 处理数组间赋值 */
Operand ir_translate_array_assign(Operand op1, Operand op2)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理数组间赋值\n");
    
    if (!op1 || !op2) {
        IR_DEBUG(IR_DEBUG_ERROR, "数组赋值操作数无效\n");
        return op1;
    }
    
    // 获取数组相关信息
    SymbolTableNode symbol1 = findSymbolInScope(op1->varName, __INT_MAX__);
    SymbolTableNode symbol2 = findSymbolInScope(op2->varName, __INT_MAX__);
    
    if (!symbol1 || !symbol2) {
        IR_DEBUG(IR_DEBUG_ERROR, "数组符号信息查找失败\n");
        return op1;
    }
    
    // 计算数组大小
    int typeSize = ir_calc_type_size(symbol2->type);
    IR_DEBUG(IR_DEBUG_VERBOSE, "数组元素大小: %d字节\n", typeSize);
    
    // 创建常量操作数
    Operand sizeOperand = ir_create_operand(CONSTANT_OP, VAL, typeSize);
    Operand byteSizeOperand = ir_create_operand(CONSTANT_OP, VAL, 4); // 4字节步长
    
    // 复制操作数并设置地址类型
    Operand srcCopy = ir_duplicate_operand(op2);
    Operand dstCopy = ir_duplicate_operand(op1);
    
    if (srcCopy->kind == VARIABLE_OP) {
        srcCopy->type = ADDRESS;
    }
    
    if (dstCopy->kind == VARIABLE_OP) {
        dstCopy->type = ADDRESS;
    }
    
    // 创建临时变量用于循环
    Operand srcPtr = ir_create_operand(TEMP_OP, VAL);
    Operand dstPtr = ir_create_operand(TEMP_OP, VAL);
    
    // 生成源和目标地址
    ir_generate_code(ASSIGN_InterCode, srcPtr, srcCopy);
    ir_generate_code(ASSIGN_InterCode, dstPtr, dstCopy);
    
    // 创建数组结束地址
    Operand varAddr = ir_create_operand(VARIABLE_OP, ADDRESS, op2->varName);
    varAddr->var_no = symbol2->var_no;
    
    Operand endAddr = ir_create_operand(TEMP_OP, VAL);
    ir_generate_code(ADD_InterCode, endAddr, varAddr, sizeOperand);
    
    // 创建循环标签
    Operand loopLabel = ir_create_operand(LABEL_OP, VAL);
    Operand exitLabel = ir_create_operand(LABEL_OP, VAL);
    
    // 生成循环代码
    ir_generate_code(LABEL_InterCode, loopLabel);
    ir_generate_code(LABEL_InterCode, srcPtr, ">=", endAddr, exitLabel);
    
    // 创建临时变量副本用于解引用
    Operand srcPtrDeref = ir_duplicate_operand(srcPtr);
    Operand dstPtrDeref = ir_duplicate_operand(dstPtr);
    srcPtrDeref->type = ADDRESS;
    dstPtrDeref->type = ADDRESS;
    
    // 复制内存
    ir_generate_code(ASSIGN_InterCode, dstPtrDeref, srcPtrDeref);
    
    // 更新指针
    ir_generate_code(ADD_InterCode, srcPtr, srcPtr, byteSizeOperand);
    ir_generate_code(ADD_InterCode, dstPtr, dstPtr, byteSizeOperand);
    
    // 继续循环
    ir_generate_code(GOTO_InterCode, loopLabel);
    
    // 循环结束标签
    ir_generate_code(LABEL_InterCode, exitLabel);
    
    return op1;
}

/* 处理逻辑表达式 */
Operand ir_translate_logical_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理逻辑表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "逻辑表达式节点为空\n");
        return NULL;
    }
    
    // 创建标签和结果变量
    Operand trueLabel = ir_create_operand(LABEL_OP, VAL);
    Operand falseLabel = ir_create_operand(LABEL_OP, VAL);
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    
    // 初始化结果为0
    Operand zeroOperand = ir_create_operand(CONSTANT_OP, VAL, 0);
    ir_generate_code(ASSIGN_InterCode, resultOperand, zeroOperand);
    
    // 翻译条件表达式，条件成立时跳转到trueLabel
    ir_translate_cond(root, trueLabel, falseLabel);
    
    // 条件为真时的代码块
    ir_generate_code(LABEL_InterCode, trueLabel);
    Operand oneOperand = ir_create_operand(CONSTANT_OP, VAL, 1);
    ir_generate_code(ASSIGN_InterCode, resultOperand, oneOperand);
    
    // 条件为假时的标签
    ir_generate_code(LABEL_InterCode, falseLabel);
    
    return resultOperand;
}

/* 处理算术表达式 */
Operand ir_translate_arithmetic_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理算术表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "算术表达式节点为空\n");
        return NULL;
    }
    
    // 获取操作符
    ASTNode *operatorNode = getChild(root, 1);
    if (!operatorNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "无法获取算术操作符\n");
        return NULL;
    }
    
    const char *opName = operatorNode->name;
    int operationType;
    
    // 确定操作类型
    if (stringComparison(opName, "PLUS")) {
        operationType = ADD_InterCode;
        IR_DEBUG(IR_DEBUG_VERBOSE, "加法运算\n");
    } else if (stringComparison(opName, "MINUS")) {
        operationType = SUB_InterCode;
        IR_DEBUG(IR_DEBUG_VERBOSE, "减法运算\n");
    } else if (stringComparison(opName, "STAR")) {
        operationType = MUL_InterCode;
        IR_DEBUG(IR_DEBUG_VERBOSE, "乘法运算\n");
    } else if (stringComparison(opName, "DIV")) {
        operationType = DIV_InterCode;
        IR_DEBUG(IR_DEBUG_VERBOSE, "除法运算\n");
    } else {
        IR_DEBUG(IR_DEBUG_ERROR, "未知的算术操作符: %s\n", opName);
        return NULL;
    }
    
    // 获取左右操作数
    ASTNode *leftExpr = getChild(root, 0);
    ASTNode *rightExpr = getChild(root, 2);
    
    if (!leftExpr || !rightExpr) {
        IR_DEBUG(IR_DEBUG_ERROR, "算术表达式的操作数不完整\n");
        return NULL;
    }
    
    // 创建结果临时变量
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    
    // 翻译左右操作数
    Operand leftOperand = ir_translate_exp(leftExpr);
    Operand rightOperand = ir_translate_exp(rightExpr);
    
    // 检查操作数有效性
    if (!leftOperand || !rightOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "算术表达式操作数求值失败: left=%p, right=%p\n", 
                 leftOperand, rightOperand);
        return resultOperand;
    }
    
    // 生成算术运算的中间代码
    ir_generate_code(operationType, resultOperand, leftOperand, rightOperand);
    
    return resultOperand;
}

/* 处理数组访问表达式 */
Operand ir_translate_array_access(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理数组访问表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "数组访问表达式节点为空\n");
        return NULL;
    }
    
    // 获取数组和索引表达式
    ASTNode *arrayExpr = getChild(root, 0);
    ASTNode *indexExpr = getChild(root, 2);
    
    if (!arrayExpr || !indexExpr) {
        IR_DEBUG(IR_DEBUG_ERROR, "数组访问表达式不完整\n");
        return NULL;
    }
    
    // 翻译数组表达式
    Operand arrayOperand = ir_translate_exp(arrayExpr);
    if (!arrayOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "数组表达式求值失败\n");
        return NULL;
    }
    
    // 复制数组操作数
    Operand arrayCopy = ir_duplicate_operand(arrayOperand);
    if (!arrayCopy) {
        IR_DEBUG(IR_DEBUG_ERROR, "复制数组操作数失败\n");
        return NULL;
    }
    
    // 获取数组维度信息
    int depth = arrayCopy->depth;
    SymbolTableNode arraySymbol = findSymbolInScope(arrayCopy->varName, __INT_MAX__);
    
    if (!arraySymbol) {
        IR_DEBUG(IR_DEBUG_ERROR, "在符号表中找不到数组: %s\n", arrayCopy->varName);
        return NULL;
    }
    
    // 分析数组维度
    Type currentType = arraySymbol->type;
    int dimensionsCount = 0;
    
    while (currentType && currentType->kind == ARRAY) {
        dimensionsCount++;
        currentType = currentType->u.array.element;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "数组 %s 有 %d 维，当前访问深度为 %d\n", 
             arrayCopy->varName, dimensionsCount, depth);
    
    // 计算元素类型大小
    int elementTypeSize = ir_calc_type_size(currentType);
    
    // 获取数组维度大小
    int *dimensionSizes = (int *)malloc(sizeof(int) * (dimensionsCount + 1));
    if (!dimensionSizes) {
        IR_DEBUG(IR_DEBUG_ERROR, "内存分配失败\n");
        return NULL;
    }
    
    // 重新遍历获取每个维度大小
    currentType = arraySymbol->type;
    int i = 0;
    while (currentType && currentType->kind == ARRAY) {
        dimensionSizes[i++] = currentType->u.array.size;
        currentType = currentType->u.array.element;
    }
    
    // 计算偏移乘数
    int index = dimensionsCount - 1;
    int depthOffset = dimensionsCount - depth - 1;
    int offsetMultiplier = 1;
    
    for (int j = 0; j < depthOffset; j++) {
        offsetMultiplier *= dimensionSizes[index--];
    }
    
    // 释放动态内存
    free(dimensionSizes);
    
    // 计算最终偏移
    offsetMultiplier *= elementTypeSize;
    
    // 翻译索引表达式
    Operand indexOperand = ir_translate_exp(indexExpr);
    if (!indexOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "索引表达式求值失败\n");
        return NULL;
    }
    
    // 创建偏移量常量
    Operand offsetMultiplierOperand = ir_create_operand(CONSTANT_OP, VAL, offsetMultiplier);
    
    // 计算偏移量
    Operand offsetOperand = ir_create_operand(TEMP_OP, VAL);
    ir_generate_code(MUL_InterCode, offsetOperand, indexOperand, offsetMultiplierOperand);
    
    // 创建结果临时变量
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    resultOperand->varName = arrayCopy->varName;
    resultOperand->depth = depth + 1;
    
    // 根据当前深度调整数组地址类型
    if (depth == 0 && arrayCopy->type == VAL) {
        arrayCopy->type = ADDRESS;
    } else {
        arrayCopy->type = VAL;
    }
    
    // 计算最终地址
    ir_generate_code(ADD_InterCode, resultOperand, arrayCopy, offsetOperand);
    
    // 复制结果并设置正确的类型
    Operand finalResult = ir_duplicate_operand(resultOperand);
    if (resultOperand->depth == dimensionsCount) {
        finalResult->type = ADDRESS;
    }
    
    return finalResult;
}

/* 处理结构体字段访问表达式 */
Operand ir_translate_field_access(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理结构体字段访问表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "结构体字段访问表达式节点为空\n");
        return NULL;
    }
    
    // 获取结构体表达式和字段名
    ASTNode *structExpr = getChild(root, 0);
    ASTNode *dotNode = getChild(root, 1);
    ASTNode *fieldNode = getChild(root, 2);
    
    if (!structExpr || !dotNode || !fieldNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "结构体字段访问表达式不完整\n");
        return NULL;
    }
    
    if (!stringComparison(fieldNode->name, "ID")) {
        IR_DEBUG(IR_DEBUG_ERROR, "字段节点不是ID类型\n");
        return NULL;
    }
    
    // 获取字段名
    const char *fieldName = fieldNode->value;
    IR_DEBUG(IR_DEBUG_VERBOSE, "访问结构体字段: %s\n", fieldName);
    
    // 翻译结构体表达式
    Operand structOperand = ir_translate_exp(structExpr);
    if (!structOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "结构体表达式求值失败\n");
        return NULL;
    }
    
    // 复制结构体操作数
    Operand structCopy = ir_duplicate_operand(structOperand);
    
    // 在符号表中查找字段信息
    SymbolTableNode fieldSymbol = findSymbolInScope(fieldName, __INT_MAX__);
    if (!fieldSymbol) {
        IR_DEBUG(IR_DEBUG_ERROR, "在符号表中找不到字段: %s\n", fieldName);
        return NULL;
    }
    
    // 获取字段偏移量
    int fieldOffset = fieldSymbol->offset;
    IR_DEBUG(IR_DEBUG_VERBOSE, "字段 %s 的偏移量为 %d\n", fieldName, fieldOffset);
    
    // 创建结果临时变量
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    
    // 根据偏移量不同采取不同处理策略
    if (fieldOffset == 0) {
        // 偏移量为0，直接使用结构体地址
        if (structCopy->type == ADDRESS) {
            structCopy->type = VAL;
        } else {
            structCopy->type = ADDRESS;
        }
        
        ir_generate_code(ASSIGN_InterCode, resultOperand, structCopy);
        
        // 设置结果属性
        Operand finalResult = ir_duplicate_operand(resultOperand);
        finalResult->type = ADDRESS;
        finalResult->varName = fieldName;
        
        return finalResult;
    } else {
        // 有偏移量，需要计算地址
        Operand offsetOperand = ir_create_operand(CONSTANT_OP, VAL, fieldOffset);
        
        if (structCopy->type == ADDRESS) {
            structCopy->type = VAL;
        } else {
            structCopy->type = ADDRESS;
        }
        
        // 计算字段地址
        ir_generate_code(ADD_InterCode, resultOperand, structCopy, offsetOperand);
        
        // 设置结果属性
        Operand finalResult = ir_duplicate_operand(resultOperand);
        finalResult->type = ADDRESS;
        finalResult->varName = fieldName;
        
        return finalResult;
    }
}

/* 处理括号表达式 */
Operand ir_translate_paren_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理括号表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "括号表达式节点为空\n");
        return NULL;
    }
    
    // 获取括号中的表达式
    ASTNode *innerExpr = getChild(root, 1);
    if (!innerExpr) {
        IR_DEBUG(IR_DEBUG_ERROR, "括号内表达式为空\n");
        return NULL;
    }
    
    // 直接翻译内部表达式
    return ir_translate_exp(innerExpr);
}

/* 处理负号表达式 */
Operand ir_translate_negative_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理负号表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "负号表达式节点为空\n");
        return NULL;
    }
    
    // 获取操作数表达式
    ASTNode *operandExpr = getChild(root, 1);
    if (!operandExpr) {
        IR_DEBUG(IR_DEBUG_ERROR, "负号表达式的操作数为空\n");
        return NULL;
    }
    
    // 创建常数0
    Operand zeroOperand = ir_create_operand(CONSTANT_OP, VAL, 0);
    
    // 翻译操作数表达式
    Operand valueOperand = ir_translate_exp(operandExpr);
    if (!valueOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "负号表达式的操作数求值失败\n");
        return NULL;
    }
    
    // 创建结果临时变量
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    
    // 生成减法中间代码：result = 0 - value
    ir_generate_code(SUB_InterCode, resultOperand, zeroOperand, valueOperand);
    
    return resultOperand;
}

/* 处理非表达式 */
Operand ir_translate_not_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理逻辑非表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "逻辑非表达式节点为空\n");
        return NULL;
    }
    
    // 创建标签
    Operand trueLabel = ir_create_operand(LABEL_OP, VAL);
    Operand falseLabel = ir_create_operand(LABEL_OP, VAL);
    
    // 创建结果临时变量
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    
    // 初始化结果为0
    Operand zeroOperand = ir_create_operand(CONSTANT_OP, VAL, 0);
    ir_generate_code(ASSIGN_InterCode, resultOperand, zeroOperand);
    
    // 翻译条件表达式，注意这里交换了true和false标签的位置
    ir_translate_cond(root, trueLabel, falseLabel);
    
    // 条件为真时（即原表达式为假时）的代码块
    ir_generate_code(LABEL_InterCode, trueLabel);
    
    // 设置结果为1
    Operand oneOperand = ir_create_operand(CONSTANT_OP, VAL, 1);
    ir_generate_code(ASSIGN_InterCode, resultOperand, oneOperand);
    
    // 条件为假时（即原表达式为真时）的标签
    ir_generate_code(LABEL_InterCode, falseLabel);
    
    return resultOperand;
}

/* 处理变量引用表达式 */
Operand ir_translate_id_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理变量引用表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "变量引用表达式节点为空\n");
        return NULL;
    }
    
    // 获取标识符节点
    ASTNode *idNode = getChild(root, 0);
    if (!idNode || !stringComparison(idNode->name, "ID")) {
        IR_DEBUG(IR_DEBUG_ERROR, "变量引用表达式中缺少ID节点\n");
        return NULL;
    }
    
    // 获取变量名
    const char *varName = idNode->value;
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理变量 %s 的引用\n", varName);
    
    // 在符号表中查找变量
    SymbolTableNode symbolNode = findSymbolInScope(varName, __INT_MAX__);
    if (!symbolNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "符号表中找不到变量 %s\n", varName);
        return NULL;
    }
    
    // 创建变量操作数
    Operand resultOperand = NULL;
    
    // 根据变量类型创建不同的操作数
    if (symbolNode->type->kind == ARRAY || symbolNode->type->kind == STRUCTURE) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "引用的是数组或结构体类型变量\n");
        
        // 根据符号的地址类型创建适当的操作数
        if (symbolNode->isAddress == ADDRESS) {
            resultOperand = ir_create_operand(VARIABLE_OP, ADDRESS, varName);
        } else {
            resultOperand = ir_create_operand(VARIABLE_OP, VAL, varName);
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "引用的是基本类型变量\n");
        resultOperand = ir_create_operand(VARIABLE_OP, VAL, varName);
    }
    
    if (!resultOperand) {
        IR_DEBUG(IR_DEBUG_ERROR, "为变量 %s 创建操作数失败\n", varName);
        return NULL;
    }
    
    // 设置操作数属性
    resultOperand->var_no = symbolNode->var_no;
    resultOperand->depth = 0;
    resultOperand->varName = varName;
    
    return resultOperand;
}

/* 处理函数调用表达式 */
Operand ir_translate_call_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理函数调用表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "函数调用表达式节点为空\n");
        return NULL;
    }
    
    // 获取函数名和参数
    ASTNode *idNode = getChild(root, 0);
    ASTNode *lpNode = getChild(root, 1);
    ASTNode *argsNode = getChild(root, 2);
    
    if (!idNode || !lpNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "函数调用表达式结构不完整\n");
        return NULL;
    }
    
    if (!stringComparison(idNode->name, "ID")) {
        IR_DEBUG(IR_DEBUG_ERROR, "函数名不是ID类型\n");
        return NULL;
    }
    
    // 获取函数名
    const char *funcName = idNode->value;
    IR_DEBUG(IR_DEBUG_VERBOSE, "调用函数: %s\n", funcName);
    
    // 创建结果临时变量
    Operand resultOperand = ir_create_operand(TEMP_OP, VAL);
    
    // 处理内置函数 write
    if (stringComparison(funcName, "write") && argsNode && stringComparison(argsNode->name, "Args")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理内置函数 write\n");
        
        // 获取write的参数
        ASTNode *writeArgExpr = getChild(argsNode, 0);
        if (!writeArgExpr) {
            IR_DEBUG(IR_DEBUG_ERROR, "write 函数缺少参数\n");
            return NULL;
        }
        
        if (stringComparison(writeArgExpr->name, "Exp")) {
            Operand argOperand = ir_translate_exp(writeArgExpr);
            if (argOperand) {
                ir_generate_code(WRITE_InterCode, argOperand);
            } else {
                IR_DEBUG(IR_DEBUG_ERROR, "write 参数求值失败\n");
            }
        } else {
            IR_DEBUG(IR_DEBUG_ERROR, "write 参数不是表达式\n");
        }
        
        // write函数返回0
        Operand zeroOperand = ir_create_operand(CONSTANT_OP, VAL, 0);
        ir_generate_code(ASSIGN_InterCode, resultOperand, zeroOperand);
        
        return resultOperand;
    }
    
    // 处理内置函数 read
    if (stringComparison(funcName, "read")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理内置函数 read\n");
        ir_generate_code(READ_InterCode, resultOperand);
        return resultOperand;
    }
    
    // 处理普通函数调用
    Operand functionOperand = ir_create_operand(FUNCTION_OP, VAL, funcName);
    
    // 检查是否有参数
    if (argsNode && stringComparison(argsNode->name, "Args")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理函数参数\n");
        
        // 查找函数信息
        SymbolTableNode funcSymbol = findSymbolInScope(funcName, __INT_MAX__);
        if (!funcSymbol) {
            IR_DEBUG(IR_DEBUG_ERROR, "在符号表中找不到函数: %s\n", funcName);
            return NULL;
        }
        
        // 翻译参数列表
        ir_translate_args(argsNode, funcSymbol->type->u.function.parameters);
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "函数无参数\n");
    }
    
    // 生成函数调用中间代码
    ir_generate_code(CALL_InterCode, resultOperand, functionOperand);
    
    return resultOperand;
}

/* 处理常量表达式 */
Operand ir_translate_constant_exp(ASTNode *root)
{
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理常量表达式 (行号: %d)\n", root->lineno);
    
    if (!root) {
        IR_DEBUG(IR_DEBUG_ERROR, "常量表达式节点为空\n");
        return NULL;
    }
    
    // 获取常量节点
    ASTNode *constNode = getChild(root, 0);
    if (!constNode) {
        IR_DEBUG(IR_DEBUG_ERROR, "常量节点为空\n");
        return NULL;
    }
    
    // 根据常量类型创建不同的操作数
    if (stringComparison(constNode->name, "INT")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理整型常量: %s\n", constNode->value);
        return ir_create_operand(CONSTANT_OP, VAL, My_atoi(constNode->value));
    } 
    else if (stringComparison(constNode->name, "FLOAT")) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "处理浮点常量: %s (在中间代码中表示为0)\n", constNode->value);
        // 浮点数在中间代码中简化为整数0处理
        return ir_create_operand(CONSTANT_OP, VAL, 0);
    } 
    else {
        IR_DEBUG(IR_DEBUG_ERROR, "未知的常量类型: %s\n", constNode->name);
        return NULL;
    }
}

/* ir_translate_args Args翻译 */
void ir_translate_args(ASTNode *root, FieldList field)
{
    /*
    Args -> Exp COMMA Args
    | Exp;
    */
    // Early return for invalid inputs
    if (!root || !field) {
        IR_DEBUG(IR_DEBUG_INFO, "Args translation: null inputs detected, returning\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_VERBOSE, "Processing function argument at line %d\n", root->lineno);
    
    // Extract child nodes
    ASTNode *exprNode = getChild(root, 0);
    ASTNode *commaNode = getChild(root, 1);
    ASTNode *nextArgsNode = getChild(root, 2);
    
    // Evaluate expression
    IR_DEBUG(IR_DEBUG_VERBOSE, "Translating expression for argument\n");
    Operand expressionResult = ir_translate_exp(exprNode);
    Operand argOperand = ir_duplicate_operand(expressionResult);
    
    // Process array and structure types which require special handling
    if (field->type->kind == STRUCTURE || field->type->kind == ARRAY) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "Processing complex type argument (array/struct)\n");
        
        int shouldUseValueType = 0;
        
        // Special handling for array types
        if (field->type->kind == ARRAY) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "Processing array type argument\n");
            
            // Get array name and lookup in symbol table
            char *arrayName = argOperand->varName;
            SymbolTableNode arraySymbol = findSymbolInScope(arrayName, __INT_MAX__);
            
            if (arraySymbol) {
                // Count array dimensions
                int dimensionCount = 0;
                Type currentType = arraySymbol->type;
                
                while (currentType && currentType->kind == ARRAY) {
                    dimensionCount++;
                    currentType = currentType->u.array.element;
                }
                
                IR_DEBUG(IR_DEBUG_VERBOSE, "Array '%s' has %d dimensions, current depth: %d\n", 
                         arrayName, dimensionCount, argOperand->depth);
                
                // Determine addressing mode based on array depth
                if (argOperand->depth < dimensionCount && argOperand->depth != 0) {
                    shouldUseValueType = 1;
                }
            } else {
                IR_DEBUG(IR_DEBUG_ERROR, "Symbol not found for array: %s\n", arrayName);
            }
        }
        
        // Set the proper type for the argument operand
        if (shouldUseValueType) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "Using VALUE type for array element\n");
            argOperand->type = VAL;
        } else if (argOperand->type == ADDRESS) {
            IR_DEBUG(IR_DEBUG_VERBOSE, "Converting ADDRESS to VALUE\n");
            argOperand->type = VAL;
        } else {
            IR_DEBUG(IR_DEBUG_VERBOSE, "Converting VALUE to ADDRESS\n");
            argOperand->type = ADDRESS;
        }
    } else {
        IR_DEBUG(IR_DEBUG_VERBOSE, "Processing simple type argument\n");
    }
    
    // Recursive call for remaining arguments (processed before generating current arg)
    if (commaNode != NULL) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "Processing next argument in list\n");
        ir_translate_args(nextArgsNode, field->nextFieldList);
    }
    
    // Generate argument code (note: processed in reverse order due to recursion)
    IR_DEBUG(IR_DEBUG_VERBOSE, "Generating ARG_InterCode for argument\n");
    ir_generate_code(ARG_InterCode, argOperand);
}

/* ir_translate_cond Cond翻译 */
void ir_translate_cond(ASTNode *root, Operand lableTure, Operand lableFalse)
{
    // 基础检查
    if (root == NULL) {
        IR_DEBUG(IR_DEBUG_ERROR, "传入的条件表达式为空\n");
        return;
    }
    
    IR_DEBUG(IR_DEBUG_INFO, "处理条件表达式 (行号: %d)\n", root->lineno);
    
    // 获取条件表达式的各个部分
    ASTNode *firstNode = getChild(root, 0);
    ASTNode *operatorNode = getChild(root, 1);
    ASTNode *secondNode = getChild(root, 2);
    
    if (firstNode == NULL) {
        IR_DEBUG(IR_DEBUG_ERROR, "条件表达式缺少第一部分\n");
        return;
    }
    
    // 处理基本节点类型
    const char *nodeName = firstNode->name;
    
    // 处理括号表达式
    if (stringComparison((char*)nodeName, "LP")) {
        ir_translate_cond(operatorNode, lableTure, lableFalse);
        return;
    }
    
    // 处理NOT表达式 - 反转标签
    if (stringComparison((char*)nodeName, "NOT")) {
        ir_translate_cond(operatorNode, lableFalse, lableTure);
        return;
    }
    
    // 处理INT常量表达式
    if (stringComparison((char*)nodeName, "INT")) {
        process_int_constant(firstNode, lableTure, lableFalse);
        return;
    }
    
    // 处理复合表达式 (Exp OPERATOR Exp)
    if (stringComparison((char*)nodeName, "Exp")) {
        // 确保有操作符
        if (operatorNode == NULL) {
            IR_DEBUG(IR_DEBUG_ERROR, "表达式缺少操作符\n");
            return;
        }
        
        const char *opName = operatorNode->name;
        
        // 处理逻辑AND
        if (stringComparison((char*)opName, "AND")) {
            process_logical_and(firstNode, secondNode, lableTure, lableFalse);
            return;
        }
        
        // 处理逻辑OR
        if (stringComparison((char*)opName, "OR")) {
            process_logical_or(firstNode, secondNode, lableTure, lableFalse);
            return;
        }
        
        // 处理关系运算符
        if (stringComparison((char*)opName, "RELOP")) {
            process_relational_op(firstNode, secondNode, operatorNode->value, lableTure, lableFalse);
            return;
        }
        
        // 处理赋值操作
        if (stringComparison((char*)opName, "ASSIGNOP")) {
            process_assignment(firstNode, secondNode, lableTure, lableFalse);
            return;
        }
        
        // 处理算术运算
        if (is_arithmetic_op(opName)) {
            process_arithmetic_expr(firstNode, secondNode, opName, lableTure, lableFalse);
            return;
        }
        
        // 处理数组或结构体访问
        if (stringComparison((char*)opName, "LB") || stringComparison((char*)opName, "DOT")) {
            process_complex_expr(root, lableTure, lableFalse);
            return;
        }
    }
    
    // 处理其他类型的表达式 (ID, MINUS等)
    process_simple_expr(root, lableTure, lableFalse);
}

/* 判断是否为算术运算符 */
static inline int is_arithmetic_op(const char *opName) {
    return stringComparison((char*)opName, "PLUS") || 
           stringComparison((char*)opName, "MINUS") || 
           stringComparison((char*)opName, "STAR") || 
           stringComparison((char*)opName, "DIV");
}

/* 处理整数常量表达式 */
static void process_int_constant(ASTNode *intNode, Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理整数常量: %s\n", intNode->value);
    
    // 解析整数值
    int value = My_atoi(intNode->value);
    
    // 根据值和目标标签生成跳转
    if (value && trueLabel) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "  整数非零，跳转到true标签\n");
        ir_generate_code(GOTO_InterCode, trueLabel);
    }
    
    if (!value && falseLabel) {
        IR_DEBUG(IR_DEBUG_VERBOSE, "  整数为零，跳转到false标签\n");
        ir_generate_code(GOTO_InterCode, falseLabel);
    }
}

/* 处理逻辑AND表达式 */
static void process_logical_and(ASTNode *leftExpr, ASTNode *rightExpr, 
                              Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理逻辑AND表达式\n");
    
    if (falseLabel) {
        // 短路求值: 左操作数为假时直接跳转到false标签
        ir_translate_cond(leftExpr, NULL, falseLabel);
        // 左操作数为真时，再计算右操作数
        ir_translate_cond(rightExpr, trueLabel, falseLabel);
    } else {
        // 没有false标签时，创建一个中间标签
        Operand midLabel = ir_create_operand(LABEL_OP, VAL);
        // 左操作数为假时跳转到中间标签
        ir_translate_cond(leftExpr, NULL, midLabel);
        // 左操作数为真时，计算右操作数
        ir_translate_cond(rightExpr, trueLabel, NULL);
        // 输出中间标签
        ir_generate_code(LABEL_InterCode, midLabel);
    }
}

/* 处理逻辑OR表达式 */
static void process_logical_or(ASTNode *leftExpr, ASTNode *rightExpr, 
                             Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理逻辑OR表达式\n");
    
    if (trueLabel) {
        // 短路求值: 左操作数为真时直接跳转到true标签
        ir_translate_cond(leftExpr, trueLabel, NULL);
        // 左操作数为假时，再计算右操作数
        ir_translate_cond(rightExpr, trueLabel, falseLabel);
    } else {
        // 没有true标签时，创建一个中间标签
        Operand midLabel = ir_create_operand(LABEL_OP, VAL);
        // 左操作数为真时跳转到中间标签
        ir_translate_cond(leftExpr, midLabel, NULL);
        // 左操作数为假时，计算右操作数
        ir_translate_cond(rightExpr, NULL, falseLabel);
        // 输出中间标签
        ir_generate_code(LABEL_InterCode, midLabel);
    }
}

/* 处理关系比较表达式 */
static void process_relational_op(ASTNode *leftExpr, ASTNode *rightExpr, char *relOp,
                                Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理关系比较表达式: %s\n", relOp);
    
    // 计算两个操作数
    Operand leftOp = ir_translate_exp(leftExpr);
    Operand rightOp = ir_translate_exp(rightExpr);
    
    if (!leftOp) {
        IR_DEBUG(IR_DEBUG_ERROR, "关系比较的左操作数为NULL\n");
        return;
    }
    
    // 根据可用的标签生成不同的跳转代码
    if (trueLabel && falseLabel) {
        // 两个标签都有时的处理
        ir_generate_code(IFGOTO_InterCode, leftOp, relOp, rightOp, trueLabel);
        ir_generate_code(GOTO_InterCode, falseLabel);
    } else if (trueLabel) {
        // 只有true标签时的处理
        ir_generate_code(IFGOTO_InterCode, leftOp, relOp, rightOp, trueLabel);
    } else if (falseLabel) {
        // 只有false标签时的处理 - 需要反转比较符
        ir_generate_code(IFGOTO_InterCode, leftOp, ir_invert_relop(relOp), rightOp, falseLabel);
    }
}

/* 处理赋值表达式 */
static void process_assignment(ASTNode *leftExpr, ASTNode *rightExpr, 
                             Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理赋值表达式\n");
    
    // 计算左右操作数
    Operand leftResult = ir_translate_exp(leftExpr);
    Operand rightResult = ir_translate_exp(rightExpr);
    
    // 生成赋值代码
    ir_generate_code(ASSIGN_InterCode, leftResult, rightResult);
    
    // 零常量用于跳转条件
    Operand zeroVal = ir_create_operand(CONSTANT_OP, VAL, 0);
    
    // 生成条件跳转代码
    if (trueLabel && falseLabel) {
        if (leftResult) {
            ir_generate_code(IFGOTO_InterCode, leftResult, "!=", zeroVal, trueLabel);
        }
        ir_generate_code(GOTO_InterCode, falseLabel);
    } else if (trueLabel) {
        if (leftResult) {
            ir_generate_code(IFGOTO_InterCode, leftResult, "!=", zeroVal, trueLabel);
        }
    } else if (falseLabel) {
        if (leftResult) {
            ir_generate_code(IFGOTO_InterCode, leftResult, "==", zeroVal, falseLabel);
        }
    }
}

/* 处理算术表达式 */
static void process_arithmetic_expr(ASTNode *leftExpr, ASTNode *rightExpr, const char *opName,
                                  Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理算术表达式: %s\n", opName);
    
    // 获取操作数
    Operand leftOp = ir_translate_exp(leftExpr);
    Operand rightOp = ir_translate_exp(rightExpr);
    
    // 确定运算类型
    int opType;
    if (stringComparison((char*)opName, "PLUS")) {
        opType = ADD_InterCode;
    } else if (stringComparison((char*)opName, "MINUS")) {
        opType = SUB_InterCode;
    } else if (stringComparison((char*)opName, "STAR")) {
        opType = MUL_InterCode;
    } else { // DIV
        opType = DIV_InterCode;
    }
    
    // 计算结果
    Operand result = ir_create_operand(TEMP_OP, VAL);
    if (leftOp && rightOp) {
        ir_generate_code(opType, result, leftOp, rightOp);
    }
    
    // 生成跳转代码
    Operand zeroVal = ir_create_operand(CONSTANT_OP, VAL, 0);
    
    if (trueLabel && falseLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "!=", zeroVal, trueLabel);
        ir_generate_code(GOTO_InterCode, falseLabel);
    } else if (trueLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "!=", zeroVal, trueLabel);
    } else if (falseLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "==", zeroVal, falseLabel);
    }
}

/* 处理复杂表达式(数组访问、结构体成员) */
static void process_complex_expr(ASTNode *expr, Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理复杂表达式(数组/结构体)\n");
    
    // 计算表达式结果
    Operand result = ir_translate_exp(expr);
    if (!result) {
        IR_DEBUG(IR_DEBUG_ERROR, "复杂表达式计算结果为NULL\n");
        return;
    }
    
    // 生成跳转代码
    Operand zeroVal = ir_create_operand(CONSTANT_OP, VAL, 0);
    
    if (trueLabel && falseLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "!=", zeroVal, trueLabel);
        ir_generate_code(GOTO_InterCode, falseLabel);
    } else if (trueLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "!=", zeroVal, trueLabel);
    } else if (falseLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "==", zeroVal, falseLabel);
    }
}

/* 处理简单表达式(ID, MINUS等) */
static void process_simple_expr(ASTNode *expr, Operand trueLabel, Operand falseLabel) {
    IR_DEBUG(IR_DEBUG_VERBOSE, "处理简单表达式: %s\n", 
             expr->firstChild ? expr->firstChild->name : "unknown");
    
    // 计算表达式结果
    Operand result = ir_translate_exp(expr);
    if (!result) {
        IR_DEBUG(IR_DEBUG_ERROR, "简单表达式计算结果为NULL\n");
        return;
    }
    
    // 生成跳转代码
    Operand zeroVal = ir_create_operand(CONSTANT_OP, VAL, 0);
    
    if (trueLabel && falseLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "!=", zeroVal, trueLabel);
        ir_generate_code(GOTO_InterCode, falseLabel);
    } else if (trueLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "!=", zeroVal, trueLabel);
    } else if (falseLabel) {
        ir_generate_code(IFGOTO_InterCode, result, "==", zeroVal, falseLabel);
    }
}

/**
 * @brief 获取反转的关系运算符
 * 
 * 将关系运算符转换为其逻辑反义符号
 * 例如: ">" 变为 "<="
 *
 * @param relop 原始关系运算符
 * @return 反转后的关系运算符
 */
char* ir_invert_relop(char* relop) {
    if (!relop) return NULL;
    
    // Map each operator to its logical inverse
    static const struct {
        const char* op;
        const char* negated;
    } relOpMap[] = {
        { ">", "<=" },
        { "<", ">=" },
        { ">=", "<" },
        { "<=", ">" },
        { "==", "!=" },
        { "!=", "==" },
        { NULL, NULL }
    };
    
    // Find the matching operator and return its negation
    for (int i = 0; relOpMap[i].op != NULL; i++) {
        if (strcmp(relop, relOpMap[i].op) == 0) {
            return (char*)relOpMap[i].negated;
        }
    }
    
    printf("Unknown relational operator: %s\n", relop);
    return NULL;
}
