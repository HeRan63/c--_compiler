#ifndef INTERMEDIATE_H
#define INTERMEDIATE_H

#include "tools.h"
#include "semantictool.h"
#include <stdarg.h>

/* 调试级别定义 */
#define IR_DEBUG_NONE 0      /* 无调试信息 */
#define IR_DEBUG_ERROR 1     /* 仅错误信息 */
#define IR_DEBUG_INFO 2      /* 常规调试信息 */
#define IR_DEBUG_VERBOSE 3   /* 详细调试信息 */

/* 全局调试级别变量 */
extern int IR_DEBUG_LEVEL;

/* 调试输出宏，根据级别输出信息 */
#define IR_DEBUG(level, ...) \
    if (IR_DEBUG_LEVEL >= level) { \
        printf("[IR-DEBUG] "); \
        printf(__VA_ARGS__); \
    }

/**
 * @brief 反转关系运算符
 * @param relationalOp 原关系运算符
 * @return 反转后的关系运算符
 */
char *ir_invert_relop(char *relationalOp);

/* 中间代码生成模块 */

/**
 * @brief 翻译整个程序
 * @param root AST根节点
 * @param file 输出文件
 */
void ir_translate_program(ASTNode *root, FILE *file);

/**
 * @brief 翻译外部定义列表
 * @param node ExtDefList节点
 */
void ir_translate_ext_def_list(ASTNode *node);

/**
 * @brief 翻译外部定义
 * @param node ExtDef节点
 */
void ir_translate_ext_def(ASTNode *node);

/**
 * @brief 翻译外部声明列表
 * @param node ExtDecList节点
 */
void ir_translate_ext_dec_list(ASTNode *node);

/**
 * @brief 翻译变量声明
 * @param node VarDec节点
 * @return 变量操作数
 */
Operand ir_translate_var_dec(ASTNode *node);

/**
 * @brief 翻译函数声明
 * @param node FunDec节点
 */
void ir_translate_fun_dec(ASTNode *node);

/**
 * @brief 翻译变量列表
 * @param node VarList节点
 */
void ir_translate_var_list(ASTNode *node);

/**
 * @brief 翻译形参声明
 * @param node ParamDec节点
 */
void ir_translate_param_dec(ASTNode *node);

/**
 * @brief 翻译复合语句
 * @param node CompSt节点
 */
void ir_translate_comp_st(ASTNode *node);

/**
 * @brief 翻译语句列表
 * @param node StmtList节点
 */
void ir_translate_stmt_list(ASTNode *node);

/**
 * @brief 翻译语句
 * @param node Stmt节点
 */
void ir_translate_stmt(ASTNode *node);

/**
 * @brief 翻译定义列表
 * @param node DefList节点
 */
void ir_translate_def_list(ASTNode *node);

/**
 * @brief 翻译定义
 * @param node Def节点
 */
void ir_translate_def(ASTNode *node);

/**
 * @brief 翻译声明列表
 * @param node DecList节点
 */
void ir_translate_dec_list(ASTNode *node);

/**
 * @brief 翻译声明
 * @param node Dec节点
 */
void ir_translate_dec(ASTNode *node);

/**
 * @brief 翻译表达式
 * @param node Exp节点
 * @return 表达式结果操作数
 */
Operand ir_translate_exp(ASTNode *node);

/**
 * @brief 处理赋值表达式
 * @param node Exp ASSIGNOP Exp节点
 * @return 表达式结果操作数
 */
Operand ir_translate_assign_exp(ASTNode *node);

/**
 * @brief 处理数组间赋值，如 arr1 = arr2
 * @param op1 左侧数组操作数
 * @param op2 右侧数组操作数
 * @return 左侧操作数
 */
Operand ir_translate_array_assign(Operand op1, Operand op2);

/**
 * @brief 处理逻辑表达式
 * @param node Exp AND/OR/RELOP Exp节点
 * @return 表达式结果操作数
 */
Operand ir_translate_logical_exp(ASTNode *node);

/**
 * @brief 处理算术表达式
 * @param node Exp PLUS/MINUS/STAR/DIV Exp节点
 * @return 表达式结果操作数
 */
Operand ir_translate_arithmetic_exp(ASTNode *node);

/**
 * @brief 处理数组访问表达式
 * @param node Exp LB Exp RB节点
 * @return 表达式结果操作数
 */
Operand ir_translate_array_access(ASTNode *node);

/**
 * @brief 处理结构体字段访问表达式
 * @param node Exp DOT ID节点
 * @return 表达式结果操作数
 */
Operand ir_translate_field_access(ASTNode *node);

/**
 * @brief 处理括号表达式
 * @param node LP Exp RP节点
 * @return 表达式结果操作数
 */
Operand ir_translate_paren_exp(ASTNode *node);

/**
 * @brief 处理负号表达式
 * @param node MINUS Exp节点
 * @return 表达式结果操作数
 */
Operand ir_translate_negative_exp(ASTNode *node);

/**
 * @brief 处理非表达式
 * @param node NOT Exp节点
 * @return 表达式结果操作数
 */
Operand ir_translate_not_exp(ASTNode *node);

/**
 * @brief 处理变量引用表达式
 * @param node ID节点
 * @return 表达式结果操作数
 */
Operand ir_translate_id_exp(ASTNode *node);

/**
 * @brief 处理函数调用表达式
 * @param node ID LP Args/RP节点
 * @return 表达式结果操作数
 */
Operand ir_translate_call_exp(ASTNode *node);

/**
 * @brief 处理常量表达式
 * @param node INT/FLOAT节点
 * @return 表达式结果操作数
 */
Operand ir_translate_constant_exp(ASTNode *node);

/**
 * @brief 翻译函数参数
 * @param node Args节点
 * @param field 形参列表
 */
void ir_translate_args(ASTNode *node, FieldList field);

/**
 * @brief 翻译条件表达式
 * @param node 条件表达式节点
 * @param label_true 条件为真时跳转的标签
 * @param label_false 条件为假时跳转的标签
 */
void ir_translate_cond(ASTNode *node, Operand label_true, Operand label_false);


/* 条件表达式处理辅助函数 */
static void process_int_constant(ASTNode *intNode, Operand trueLabel, Operand falseLabel);
static void process_logical_and(ASTNode *leftExpr, ASTNode *rightExpr, Operand trueLabel, Operand falseLabel);
static void process_logical_or(ASTNode *leftExpr, ASTNode *rightExpr, Operand trueLabel, Operand falseLabel);
static void process_relational_op(ASTNode *leftExpr, ASTNode *rightExpr, char *relOp, Operand trueLabel, Operand falseLabel);
static void process_assignment(ASTNode *leftExpr, ASTNode *rightExpr, Operand trueLabel, Operand falseLabel);
static void process_arithmetic_expr(ASTNode *leftExpr, ASTNode *rightExpr, const char *opName, Operand trueLabel, Operand falseLabel);
static void process_complex_expr(ASTNode *expr, Operand trueLabel, Operand falseLabel);
static void process_simple_expr(ASTNode *expr, Operand trueLabel, Operand falseLabel);
static inline int is_arithmetic_op(const char *opName);

#endif /* INTERMEDIATE_H */