# 实验四：目标代码生成

#### 贺然 221240014

已完成所有必做和选做部分：
## 使用方法

1. **编译项目**：
```bash
make
```

2. **运行编译器**：
```bash
./parser test.cmm test.s
```
- `test.cmm`: 输入的 C-- 源代码文件
- `test.s`: 输出的 MIPS 汇编代码文件

3. **调试选项**：
在 `mips.h` 中可以设置调试选项：
```c
#define MIPS_DEBUG 1  // 启用调试输出
```


##  核心组件

### 2.1 寄存器管理
```c
typedef struct MipsRegister {
    bool isOccupied;                    
    char *regName;                      
    MipsRegisterAllocation varAlloc;    
} MipsRegister;
```
- 维护寄存器使用状态
- 处理寄存器分配和释放
- 管理变量到寄存器的映射

### 2.2 内存管理
```c
typedef struct MipsRegisterAllocation_ {
    char name[MAX_NAME_LENGTH];         
    int regNum;                         
    int stackOffset;                    
    MipsRegisterAllocation next;        
} MipsRegisterAllocation_;
```
- 管理栈帧布局
- 处理变量内存分配
- 维护变量偏移量

## 3. 主要功能模块

### 3.1 代码生成主流程
```c
void generateMipsCode(FILE *file)
```
- 遍历中间代码
- 根据指令类型调用相应生成函数
- 处理错误和调试信息

### 3.2 函数处理
```c
void generateMipsFunction(InterCodes curInterCodes, FILE *file)
```
- 生成函数序言和尾声
- 处理参数传递
- 管理局部变量分配

### 3.3 指令生成
- 算术运算：`generateMipsAdd`, `generateMipsSub` 等
- 控制流：`generateMipsGoto`, `generateMipsIfGoto`
- 函数调用：`generateMipsArg`
- I/O 操作：`generateMipsRead`, `generateMipsWrite`

## 4. 关键特性

### 4.1 调试支持
```c
#define MIPS_DEBUG_PRINT(fmt, ...) \
    do { if (MIPS_DEBUG) fprintf(stderr, DEBUG_PREFIX fmt "\n", ##__VA_ARGS__); } while(0)
```
- 详细的调试信息输出
- 可配置的调试级别
- 清晰的错误追踪

### 4.2 错误处理
- 输入参数验证
- 内存分配检查
- 寄存器使用验证
- 完整的错误报告

### 4.3 优化考虑
- 基本的寄存器分配策略
- 简单的指令选择优化
- 栈帧优化

## 5. 注意事项

1. 寄存器使用规范：
   - `$t0-$t7`: 临时寄存器
   - `$s0-$s7`: 保存寄存器
   - `$ra`: 返回地址
   - `$fp`: 帧指针
   - `$sp`: 栈指针

2. 内存对齐：
   - 所有数据按 4 字节对齐
   - 栈指针操作保持对齐

3. 函数调用约定：
   - 参数通过栈传递
   - 返回值使用 `$v0`
   - 保护调用者寄存器
