#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
    false = 0,
    true = 1
} bool;

#define TABLESIZE 0x3fff

typedef struct HashTableNode_ *HashTableNode;
typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;
typedef struct SymbolTableNode_ *SymbolTableNode;
typedef struct FunctionTable_ *FunctionTable;
typedef struct Operand_ *Operand;
typedef struct InterCodes_ *InterCodes;

/* 抽象语法树节点类型 */
typedef enum {
    NODE_TYPE_NON_TERMINAL = 0,  // 非终结符节点
    NODE_TYPE_TERMINAL = 1,      // 终结符节点
    NODE_TYPE_TOKEN = 2          // 词法单元节点
} ASTNodeType;

/* 抽象语法树节点结构 */
typedef struct ASTNode {
    int lineno;                     // 行号
    ASTNodeType type;              // 节点类型
    char* name;                    // 节点名称
    char* value;                   // 节点值
    struct ASTNode* firstChild;    // 第一个子节点
    struct ASTNode* nextSibling;   // 下一个兄弟节点
} ASTNode;

/* Type_ 节点信息 */
typedef struct Type_
{
    enum
    {
        BASIC,
        ARRAY,
        STRUCTURE,
        FUNCTION
    } kind;
    union
    {
        int basic; //基本类型:0为int, 1为float
        struct
        {
            int size;     //数组大小
            Type element; //元素类型
        } array;
        struct
        { //数组类型信息
            char *name;
            FieldList structures;
        } structure; //结构体类型信息
        struct
        {
            int parameterNum; //参数个数
            FieldList parameters;
            Type returnType; //返回值类型
        } function;          //函数类型信息
    } u;
} Type_;

/* FieldList_ 域信息 */
typedef struct FieldList_
{
    char *name;              //域的名字
    Type type;               //域的类型
    FieldList nextFieldList; //下一个域
} FieldList_;

/* SymbolTableNode_ 符号表节点 */
typedef struct SymbolTableNode_
{
    Type type;
    char *name;
    int kind;       // 0 var 1 struct 2 function
    bool isDefined; //是否定义
    int depth;
    int var_no;
    int isAddress;
    int offset;
    char *structName;
    SymbolTableNode sameHashSymbolTableNode;
    SymbolTableNode controlScopeSymbolTableNode;
} SymbolTableNode_;

/* HashTable 符号表 */
typedef struct HashTableNode_
{
    SymbolTableNode symbolTableNode;
    HashTableNode nextHashTableNode;
} HashTableNode_;

/* FunctionTable 函数表 */
struct FunctionTable_
{
    char *name;
    int functionLineNumber;
    FunctionTable next;
};

/* SemanticError 错误类型 */
typedef enum SemanticError
{
    Undefined_Variable,
    Undefined_Function,
    Redefined_Variable_Name,
    Redefined_Function,
    AssignOP_Type_Dismatch,
    Leftside_Rvalue_Error,
    Operand_Type_Dismatch,
    Return_Type_Dismatch,
    Func_Call_Parameter_Dismatch,
    Operate_Others_As_Array,
    Operate_Basic_As_Func,
    Array_Float_Index,
    Operate_Others_As_Struct,
    Undefined_Field,
    Redefined_Field,
    Redefined_Field_Name,
    Undefined_Struct,
    Undefined_Function_But_Declaration,
    Conflict_Decordef_Funcion
} SemanticError;

struct Operand_
{
    enum
    {
        VARIABLE_OP, //变量
        CONSTANT_OP, //常量
        TEMP_OP,     //临时变量
        FUNCTION_OP, //函数
        LABEL_OP     //标号
    } kind;
    enum
    {
        VAL,
        ADDRESS
    } type;
    int var_no;     //标号数
    int value;      //数值
    char *varName;  //变量名
    char *funcName; //函数名
    int depth;
} Operand_;

struct InterCode
{
    enum
    {
        LABEL_InterCode,       //定义标号x LABEL x :
        FUNC_InterCode,        //定义函数f FUNCTION f :
        ASSIGN_InterCode,      //赋值操作 x := y
        ADD_InterCode,         //加法操作 x := y + z
        SUB_InterCode,         //减法操作 x := y - z
        MUL_InterCode,         //乘法操作 x := y * z
        DIV_InterCode,         //除法操作 x := y / z
        GET_ADDR_InterCode,    //取y的地址赋给x x := &y
        GET_CONTENT_InterCode, //取以y值为地址的内存单元的内容赋给x x := *y
        TO_ADDR_InterCode,     //取y值赋给以x值为地址的内存单元 *x := y
        GOTO_InterCode,        //无条件跳转至标号x GOTO x
        IFGOTO_InterCode,      //如果x与y满足[relop]关系则跳转至标号z IF x [relop] y GOTO z
        RETURN_InterCode,      //退出当前函数并返回x值 RETURN x
        DEC_InterCode,         //内存空间申请，大小为4的倍数 DEC x [size]
        ARG_InterCode,         //传实参x ARG x
        CALL_InterCode,        //调用函数，并将其返回值赋给x x := CALL f
        PARAM_InterCode,       //函数参数声明 PARAM x
        READ_InterCode,        //从控制台读取x的值 READ x
        WRITE_InterCode        //向控制台打印x的值 WRITE x
    } kind;
    union
    {
        struct
        {
            Operand op;
        } singleOP; // LABEL FUNC GOTO RETURN ARG PARAM READ WRITE
        struct
        {
            Operand left, right;
        } doubleOP; // ASSIGN GET_ADDR GET_CONTENT TO_ADDR CALL DEC
        struct
        {
            Operand result, op1, op2;
        } tripleOP; // ADD SUB MUL DIV
        struct
        {
            Operand op1, op2, label;
            char *relop;
        } ifgotoOP; // IFGOTO
    } u;
} InterCode;

struct InterCodes_
{
    struct InterCode code;
    InterCodes prev, next;
} InterCodes_; //双向链表

extern ASTNode* ast_root;
extern int yylineno;
extern int varNo;
extern int tempNo;
extern int labelNo;
extern InterCodes interCodeListHead;
extern InterCodes interCodeListTail;

/* AST节点操作函数 */
ASTNode* ast_create_node(const char* name, const char* value, ASTNodeType type, int lineno);
void ast_add_child(ASTNode* parent, int num_children, ...);
void ast_destroy(ASTNode* root);
void ast_print(ASTNode* root, int depth);

void print_node_info(const char* name, const char* value);
int parse_number(const char* str);

/* 基本工具函数 */
bool stringComparison(char *src, char *dst);
int to_Oct(char *str);
int to_Hex(char *str);
int to_Dec(char *str);
char* ita(int num, char *str);
int My_atoi(char *str);
unsigned int hash_pjw(char *name);
ASTNode *getChild(ASTNode *root, int childnum);

/* 中间代码相关函数 */
/* 生成新的中间代码节点 */
void ir_generate_code(int opKind, ...);
/* 构造操作数对象 */
Operand ir_create_operand(int opKind, int dataType, ...);
/* 格式化输出操作数 */
void ir_output_operand(Operand op, FILE *outFile);
/* 输出所有中间代码到文件 */
void ir_write_codes(FILE *outFile);
/* 创建操作数的深拷贝 */
Operand ir_duplicate_operand(Operand source);

/* 计算类型占用的内存空间大小 */
int ir_calc_type_size(Type dataType);

#endif