#include "mips.h"
#define MIPS_PRELUDE ".data\n_prompt: .asciiz \"\"\n_ret: .asciiz \"\\n\"\n.globl main\n.text\n" \
                "read:\n\tli $v0, 4\n\tla $a0, _prompt\n\tsyscall\n\tli $v0, 5\n\tsyscall\n\tjr $ra\n\n"     \
                "write:\n\tli $v0, 1\n\tsyscall\n\tli $v0, 4\n\tla $a0, _ret\n\tsyscall\n\tmove $v0, $0\n\tjr $ra\n\n"
// #define PRECODE ".data\n_prompt: .asciiz \"Enter an integer:\"\n_ret: .asciiz \"\\n\"\n.globl main\n.text\n" \
//                 "read:\n\tli $v0, 4\n\tla $a0, _prompt\n\tsyscall\n\tli $v0, 5\n\tsyscall\n\tjr $ra\n\n"     \
//                 "write:\n\tli $v0, 1\n\tsyscall\n\tli $v0, 4\n\tla $a0, _ret\n\tsyscall\n\tmove $v0, $0\n\tjr $ra\n\n"

MipsRegister mipsRegisters[32];
char *mipsRegNames[32] = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

int currentStackOffset = 0;
MipsRegisterAllocation varAllocationList = NULL;

/* Control flow related code generation */

// Mapping of relational operators to MIPS branch instructions
typedef struct {
    const char* relop;
    const char* mipsInstr;
} RelOpMapping;

static const RelOpMapping RELOP_MAP[] = {
    {"==", "beq"},
    {"!=", "bne"},
    {">", "bgt"},
    {"<", "blt"},
    {">=", "bge"},
    {"<=", "ble"},
    {NULL, NULL}
};

/* Register management constants */
#define TEMP_REG_START 8
#define TEMP_REG_END 15
#define MAX_NAME_LENGTH 32

/* Helper function to create variable name for temporary variables */
static void createTempVarName(int varNo, char* buffer, size_t bufferSize) {
    if (buffer && bufferSize > 0) {
        char numStr[16];
        ita(varNo, numStr);
        snprintf(buffer, bufferSize, "t%s", numStr);
    }
}

/* Main MIPS code generation function */
void generateMipsCode(FILE *file) {
    if (!file) {
        MIPS_DEBUG_PRINT("Error: Invalid file pointer");
        return;
    }

    if (!interCodeListHead) {
        MIPS_DEBUG_PRINT("Error: No intermediate code to process");
        return;
    }

    MIPS_DEBUG_PRINT("Starting MIPS code generation");
    
    // Initialize registers and write prelude
    initMipsRegisters();
    fprintf(file, MIPS_PRELUDE);
    
    // Process all intermediate codes
    InterCodes curInterCodes = interCodeListHead->next;
    while (curInterCodes != interCodeListHead) {
        MIPS_DEBUG_PRINT("Processing intermediate code of type: %d", curInterCodes->code.kind);
        
        switch (curInterCodes->code.kind) {
            case LABEL_InterCode: {
                int labelNo = curInterCodes->code.u.singleOP.op->var_no;
                fprintf(file, "label%d:\n", labelNo);
                MIPS_DEBUG_PRINT("Generated label%d", labelNo);
                break;
            }
            
            case FUNC_InterCode:
                generateMipsFunction(curInterCodes, file);
                break;
                
            case ASSIGN_InterCode:
                generateMipsAssignment(curInterCodes, file);
                break;
                
            case ADD_InterCode:
            case SUB_InterCode:
            case MUL_InterCode:
            case DIV_InterCode: {
                // Group arithmetic operations
                switch (curInterCodes->code.kind) {
                    case ADD_InterCode: generateMipsAdd(curInterCodes, file); break;
                    case SUB_InterCode: generateMipsSub(curInterCodes, file); break;
                    case MUL_InterCode: generateMipsMul(curInterCodes, file); break;
                    case DIV_InterCode: generateMipsDiv(curInterCodes, file); break;
                }
                break;
            }
                
            case GOTO_InterCode:
                generateMipsGoto(curInterCodes, file);
                break;
                
            case IFGOTO_InterCode:
                generateMipsIfGoto(curInterCodes, file);
                break;
                
            case RETURN_InterCode:
                generateMipsReturn(curInterCodes, file);
                break;
                
            case ARG_InterCode: {
                generateMipsArg(curInterCodes, file);
                // Skip to after CALL instruction
                while (curInterCodes && curInterCodes->code.kind != CALL_InterCode) {
                    curInterCodes = curInterCodes->next;
                }
                if (!curInterCodes) {
                    MIPS_DEBUG_PRINT("Error: ARG without matching CALL");
                    return;
                }
                break;
            }
                
            case READ_InterCode:
                generateMipsRead(curInterCodes, file);
                break;
                
            case WRITE_InterCode:
                generateMipsWrite(curInterCodes, file);
                break;
                
            default:
                MIPS_DEBUG_PRINT("Warning: Unhandled intermediate code type: %d", 
                    curInterCodes->code.kind);
                break;
        }
        
        curInterCodes = curInterCodes->next;
    }
    
    MIPS_DEBUG_PRINT("MIPS code generation completed");
}

/* Initialize MIPS registers */
void initMipsRegisters()
{
    MIPS_DEBUG_PRINT("Initializing MIPS registers");
    
    // Initialize register names
    for (int i = 0; i < 32; i++) {
        mipsRegisters[i].regName = mipsRegNames[i];
        MIPS_DEBUG_PRINT("Register %d initialized with name: %s", i, mipsRegNames[i]);
    }
    
    // Initialize register states
    for (int i = 0; i < 32; i++) {
        mipsRegisters[i].isOccupied = 0;
        mipsRegisters[i].varAlloc = NULL;
    }
    
    MIPS_DEBUG_PRINT("All registers initialized");
}

/* Find and allocate a suitable register for an operand */
int allocateMipsRegister(Operand op, FILE *file)
{
    if (!op || !file) {
        MIPS_DEBUG_PRINT("Error: Invalid parameters in allocateMipsRegister");
        return 0;
    }

    MIPS_DEBUG_PRINT("Allocating register for operand type: %d", op->kind);

    // Handle constant operands
    if (op->kind == CONSTANT_OP) {
        MIPS_DEBUG_PRINT("Handling constant value: %d", op->value);
        
        for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
            if (!mipsRegisters[i].isOccupied) {
                mipsRegisters[i].isOccupied = 1;
                mipsRegisters[i].varAlloc = NULL;
                fprintf(file, "\tli %s, %d\n", mipsRegisters[i].regName, op->value);
                
                MIPS_DEBUG_PRINT("Allocated register %s for constant %d", 
                    mipsRegisters[i].regName, op->value);
                return i;
            }
        }
        
        MIPS_DEBUG_PRINT("Error: No free register available for constant");
        return 0;
    }

    // Handle variables and temporaries
    for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
        if (!mipsRegisters[i].isOccupied) {
            mipsRegisters[i].isOccupied = 1;
            
            // Get variable allocation information
            MipsRegisterAllocation varAlloc = getMipsVarAllocation(op);
            if (!varAlloc) {
                MIPS_DEBUG_PRINT("Error: Failed to get variable allocation");
                mipsRegisters[i].isOccupied = 0;
                return 0;
            }
            
            varAlloc->regNum = i;
            mipsRegisters[i].varAlloc = varAlloc;
            
            // Generate appropriate load instruction based on operand type
            if (op->kind == TEMP_OP && op->type == ADDRESS) {
                // Handle pointer dereference
                MIPS_DEBUG_PRINT("Loading pointer value at offset %d", varAlloc->stackOffset);
                fprintf(file, "\tlw %s, %d($fp)\n", mipsRegisters[i].regName, varAlloc->stackOffset);
                fprintf(file, "\tlw %s, 0(%s)\n", mipsRegisters[i].regName, mipsRegisters[i].regName);
            }
            else if (op->kind == VARIABLE_OP && op->type == ADDRESS) {
                // Handle address-of operation
                MIPS_DEBUG_PRINT("Computing address at offset %d", varAlloc->stackOffset);
                fprintf(file, "\taddi %s, $fp, %d\n", mipsRegisters[i].regName, varAlloc->stackOffset);
            }
            else {
                // Handle regular variable load
                MIPS_DEBUG_PRINT("Loading value from offset %d", varAlloc->stackOffset);
                fprintf(file, "\tlw %s, %d($fp)\n", mipsRegisters[i].regName, varAlloc->stackOffset);
            }
            
            return i;
        }
    }
    
    MIPS_DEBUG_PRINT("Error: No free register available");
    return 0;
}

/* Get variable allocation information */
MipsRegisterAllocation getMipsVarAllocation(Operand op)
{
    if (!op) {
        MIPS_DEBUG_PRINT("Error: Invalid operand in getMipsVarAllocation");
        return NULL;
    }

    MIPS_DEBUG_PRINT("Looking up allocation for operand type: %d", op->kind);
    
    MipsRegisterAllocation curAlloc = varAllocationList;
    while (curAlloc) {
        if (op->kind == VARIABLE_OP) {
            if (strcmp(curAlloc->name, op->varName) == 0) {
                MIPS_DEBUG_PRINT("Found allocation for variable %s", op->varName);
                return curAlloc;
            }
        }
        else if (op->kind == TEMP_OP) {
            char tempName[MAX_NAME_LENGTH];
            createTempVarName(op->var_no, tempName, sizeof(tempName));
            
            if (strcmp(curAlloc->name, tempName) == 0) {
                MIPS_DEBUG_PRINT("Found allocation for temporary %s", tempName);
                return curAlloc;
            }
        }
        curAlloc = curAlloc->next;
    }
    
    MIPS_DEBUG_PRINT("No allocation found for operand");
    return NULL;
}

/* Store register value back to stack */
void storeMipsRegisterToStack(int regIndex, FILE *file)
{
    if (regIndex < TEMP_REG_START || regIndex > TEMP_REG_END || !file) {
        MIPS_DEBUG_PRINT("Error: Invalid parameters in storeMipsRegisterToStack");
        return;
    }

    if (!mipsRegisters[regIndex].varAlloc) {
        MIPS_DEBUG_PRINT("Error: No allocation information for register %s", 
            mipsRegisters[regIndex].regName);
        return;
    }

    MIPS_DEBUG_PRINT("Storing register %s back to stack", mipsRegisters[regIndex].regName);
    
    // Store value back to stack
    int offset = mipsRegisters[regIndex].varAlloc->stackOffset;
    fprintf(file, "\tsw %s, %d($fp)\n", mipsRegisters[regIndex].regName, offset);
    
    // Free all temporary registers
    for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
        if (mipsRegisters[i].isOccupied) {
            mipsRegisters[i].isOccupied = 0;
            MIPS_DEBUG_PRINT("Freed register %s", mipsRegisters[i].regName);
        }
    }
}

/* Create new variable allocation */
void createMipsVarAllocation(Operand op)
{
    if (!op) {
        MIPS_DEBUG_PRINT("Error: Invalid operand in createMipsVarAllocation");
        return;
    }

    if (op->kind == CONSTANT_OP) {
        MIPS_DEBUG_PRINT("Skipping allocation for constant");
        return;
    }

    MIPS_DEBUG_PRINT("Creating allocation for operand type: %d", op->kind);
    
    // Check if allocation already exists
    MipsRegisterAllocation tempAlloc = getMipsVarAllocation(op);
    if (tempAlloc) {
        MIPS_DEBUG_PRINT("Allocation already exists");
        return;
    }

    // Create new allocation
    currentStackOffset += 4;
    MipsRegisterAllocation newAlloc = (MipsRegisterAllocation)malloc(sizeof(MipsRegisterAllocation_));
    if (!newAlloc) {
        MIPS_DEBUG_PRINT("Error: Memory allocation failed");
        return;
    }

    // Initialize allocation
    if (op->kind == VARIABLE_OP) {
        strncpy(newAlloc->name, op->varName, MAX_NAME_LENGTH - 1);
        newAlloc->name[MAX_NAME_LENGTH - 1] = '\0';
        MIPS_DEBUG_PRINT("Created allocation for variable %s", op->varName);
    }
    else if (op->kind == TEMP_OP) {
        createTempVarName(op->var_no, newAlloc->name, MAX_NAME_LENGTH);
        MIPS_DEBUG_PRINT("Created allocation for temporary %s", newAlloc->name);
    }

    newAlloc->stackOffset = -currentStackOffset;
    newAlloc->next = varAllocationList;
    varAllocationList = newAlloc;
    
    MIPS_DEBUG_PRINT("Allocation created at offset %d", newAlloc->stackOffset);
}

/* Function prologue and epilogue generation */
static void generateFunctionPrologue(const char* funcName, FILE* file) {
    MIPS_DEBUG_PRINT("Generating prologue for function: %s", funcName);
    
    // Function label
    fprintf(file, "%s:\n", funcName);
    
    // Save frame pointer and return address
    fprintf(file, "\taddi $sp, $sp, -8\n");
    fprintf(file, "\tsw $fp, 0($sp)\n");
    fprintf(file, "\tsw $ra, 4($sp)\n");
    
    // Set up new frame pointer
    fprintf(file, "\tmove $fp, $sp\n");
    
    MIPS_DEBUG_PRINT("Function prologue completed");
}

/* Parameter and local variable allocation */
static void allocateParameters(InterCodes* curInterCodes, int* paramCount, FILE* file) {
    MIPS_DEBUG_PRINT("Allocating parameters");
    
    while ((*curInterCodes)->code.kind == PARAM_InterCode) {
        MipsRegisterAllocation param = (MipsRegisterAllocation)malloc(sizeof(MipsRegisterAllocation_));
        if (!param) {
            MIPS_DEBUG_PRINT("Error: Memory allocation failed for parameter");
            return;
        }
        
        strcpy(param->name, (*curInterCodes)->code.u.singleOP.op->varName);
        param->stackOffset = 8 + (*paramCount) * 4;
        param->next = varAllocationList;
        varAllocationList = param;
        
        MIPS_DEBUG_PRINT("Allocated parameter %s at offset %d", 
            param->name, param->stackOffset);
        
        (*paramCount)++;
        *curInterCodes = (*curInterCodes)->next;
    }
}

/* Local variable allocation for different instruction types */
static void allocateLocalVars(InterCodes curInterCodes) {
    MIPS_DEBUG_PRINT("Allocating local variables");
    
    while (curInterCodes != NULL && curInterCodes->code.kind != FUNC_InterCode) {
        switch (curInterCodes->code.kind) {
            case ASSIGN_InterCode:
                createMipsVarAllocation(curInterCodes->code.u.doubleOP.left);
                createMipsVarAllocation(curInterCodes->code.u.doubleOP.right);
                break;
                
            case ADD_InterCode:
            case SUB_InterCode:
            case MUL_InterCode:
            case DIV_InterCode:
                createMipsVarAllocation(curInterCodes->code.u.tripleOP.op1);
                createMipsVarAllocation(curInterCodes->code.u.tripleOP.op2);
                createMipsVarAllocation(curInterCodes->code.u.tripleOP.result);
                break;
                
            case DEC_InterCode: {
                currentStackOffset += curInterCodes->code.u.doubleOP.right->value;
                MipsRegisterAllocation array = (MipsRegisterAllocation)malloc(sizeof(MipsRegisterAllocation_));
                if (!array) {
                    MIPS_DEBUG_PRINT("Error: Memory allocation failed for array");
                    return;
                }
                strcpy(array->name, curInterCodes->code.u.doubleOP.left->varName);
                array->stackOffset = (-1) * currentStackOffset;
                array->next = varAllocationList;
                varAllocationList = array;
                MIPS_DEBUG_PRINT("Allocated array %s at offset %d", 
                    array->name, array->stackOffset);
                break;
            }
                
            case IFGOTO_InterCode:
                createMipsVarAllocation(curInterCodes->code.u.ifgotoOP.op1);
                createMipsVarAllocation(curInterCodes->code.u.ifgotoOP.op2);
                break;
                
            case CALL_InterCode:
                createMipsVarAllocation(curInterCodes->code.u.doubleOP.left);
                break;
                
            case ARG_InterCode:
            case WRITE_InterCode:
            case READ_InterCode:
                createMipsVarAllocation(curInterCodes->code.u.singleOP.op);
                break;
        }
        curInterCodes = curInterCodes->next;
    }
}

/* Function definition code generation */
void generateMipsFunction(InterCodes curInterCodes, FILE *file)
{
    const char* funcName = curInterCodes->code.u.singleOP.op->funcName;
    MIPS_DEBUG_PRINT("Generating code for function: %s", funcName);
    
    // Generate function prologue
    generateFunctionPrologue(funcName, file);
    
    // Initialize stack frame
    currentStackOffset = 0;
    int paramCount = 0;
    
    // Process parameters
    InterCodes tmpInterCodes = curInterCodes->next;
    allocateParameters(&tmpInterCodes, &paramCount, file);
    
    // Allocate local variables
    allocateLocalVars(tmpInterCodes);
    
    // Adjust stack pointer for local variables
    if (currentStackOffset > 0) {
        fprintf(file, "\taddi $sp, $sp, %d\n", (-1) * currentStackOffset);
        MIPS_DEBUG_PRINT("Adjusted stack pointer by %d bytes", (-1) * currentStackOffset);
    }
    
    // Free temporary registers
    for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
        if (mipsRegisters[i].isOccupied) {
            mipsRegisters[i].isOccupied = 0;
            MIPS_DEBUG_PRINT("Freed temporary register %s", mipsRegisters[i].regName);
        }
    }
}

/* Common arithmetic operation generation */
static void generateMipsArithmeticOp(InterCodes curInterCodes, FILE *file, const char* opcode) {
    MIPS_DEBUG_PRINT("Generating %s operation", opcode);
    
    // Allocate registers for operands and result
    int resultIndex = allocateMipsRegister(curInterCodes->code.u.tripleOP.result, file);
    int op1Index = allocateMipsRegister(curInterCodes->code.u.tripleOP.op1, file);
    int op2Index = allocateMipsRegister(curInterCodes->code.u.tripleOP.op2, file);
    
    // Generate arithmetic instruction
    fprintf(file, "\t%s %s, %s, %s\n",
        opcode,
        mipsRegisters[resultIndex].regName,
        mipsRegisters[op1Index].regName,
        mipsRegisters[op2Index].regName);
    
    // Store result back to memory
    storeMipsRegisterToStack(resultIndex, file);
    
    MIPS_DEBUG_PRINT("%s operation completed: %s = %s %s %s",
        opcode,
        mipsRegisters[resultIndex].regName,
        mipsRegisters[op1Index].regName,
        opcode,
        mipsRegisters[op2Index].regName);
}

/* Assignment code generation */
void generateMipsAssignment(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating assignment operation");
    
    Operand leftOp = curInterCodes->code.u.doubleOP.left;
    Operand rightOp = curInterCodes->code.u.doubleOP.right;
    
    int rightIndex = allocateMipsRegister(rightOp, file);
    int leftIndex = TEMP_REG_START;
    
    if (leftOp->kind == TEMP_OP && leftOp->type == ADDRESS) {
        // Handle pointer assignment (*x = y)
        MIPS_DEBUG_PRINT("Handling pointer assignment");
        
        MipsRegisterAllocation leftVarAlloc = getMipsVarAllocation(leftOp);
        
        // Find a free register
        for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
            if (!mipsRegisters[i].isOccupied) {
                leftIndex = i;
                break;
            }
        }
        
        mipsRegisters[leftIndex].isOccupied = 1;
        
        // Load address and store value
        fprintf(file, "\tlw %s, %d($fp)\n", 
            mipsRegisters[leftIndex].regName, 
            leftVarAlloc->stackOffset);
        fprintf(file, "\tsw %s, 0(%s)\n", 
            mipsRegisters[rightIndex].regName, 
            mipsRegisters[leftIndex].regName);
            
        // Free registers
        mipsRegisters[leftIndex].isOccupied = 0;
        mipsRegisters[rightIndex].isOccupied = 0;
        
        MIPS_DEBUG_PRINT("Pointer assignment completed");
    }
    else {
        // Handle regular assignment (x = y)
        MIPS_DEBUG_PRINT("Handling regular assignment");
        
        leftIndex = allocateMipsRegister(leftOp, file);
        fprintf(file, "\tmove %s, %s\n", 
            mipsRegisters[leftIndex].regName, 
            mipsRegisters[rightIndex].regName);
        storeMipsRegisterToStack(leftIndex, file);
        
        MIPS_DEBUG_PRINT("Regular assignment completed");
    }
}

/* Arithmetic operations code generation */
void generateMipsAdd(InterCodes curInterCodes, FILE *file) {
    generateMipsArithmeticOp(curInterCodes, file, "add");
}

void generateMipsSub(InterCodes curInterCodes, FILE *file) {
    generateMipsArithmeticOp(curInterCodes, file, "sub");
}

void generateMipsMul(InterCodes curInterCodes, FILE *file) {
    generateMipsArithmeticOp(curInterCodes, file, "mul");
}

void generateMipsDiv(InterCodes curInterCodes, FILE *file) {
    generateMipsArithmeticOp(curInterCodes, file, "div");
}

/* Generate unconditional jump */
void generateMipsGoto(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating unconditional jump to label%d", 
        curInterCodes->code.u.singleOP.op->var_no);
    
    fprintf(file, "\tj label%d\n", curInterCodes->code.u.singleOP.op->var_no);
}

/* Generate conditional branch based on comparison */
void generateMipsIfGoto(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating conditional branch");
    
    // Load operands into registers
    int op1Index = allocateMipsRegister(curInterCodes->code.u.ifgotoOP.op1, file);
    int op2Index = allocateMipsRegister(curInterCodes->code.u.ifgotoOP.op2, file);
    
    const char* relop = curInterCodes->code.u.ifgotoOP.relop;
    int labelNo = curInterCodes->code.u.ifgotoOP.label->var_no;
    
    MIPS_DEBUG_PRINT("Condition: %s %s %s, jumping to label%d",
        mipsRegisters[op1Index].regName,
        relop,
        mipsRegisters[op2Index].regName,
        labelNo);

    // Find the corresponding MIPS branch instruction
    const RelOpMapping* mapping = RELOP_MAP;
    while (mapping->relop != NULL) {
        if (strcmp(relop, mapping->relop) == 0) {
            fprintf(file, "\t%s %s, %s, label%d\n",
                mapping->mipsInstr,
                mipsRegisters[op1Index].regName,
                mipsRegisters[op2Index].regName,
                labelNo);
            break;
        }
        mapping++;
    }

    if (mapping->relop == NULL) {
        MIPS_DEBUG_PRINT("Error: Unknown relational operator: %s", relop);
    }

    // Free registers
    mipsRegisters[op1Index].isOccupied = 0;
    mipsRegisters[op2Index].isOccupied = 0;
    
    MIPS_DEBUG_PRINT("Released registers %s and %s",
        mipsRegisters[op1Index].regName,
        mipsRegisters[op2Index].regName);
}

/* Generate function return code */
void generateMipsReturn(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating return statement");

    // Restore return address and frame pointer
    fprintf(file, "\tlw $ra, 4($fp)\n");
    fprintf(file, "\taddi $sp, $fp, 8\n");
    
    // Load return value into $v0
    int returnValueReg = allocateMipsRegister(curInterCodes->code.u.singleOP.op, file);
    MIPS_DEBUG_PRINT("Return value loaded into %s", mipsRegisters[returnValueReg].regName);
    
    // Restore frame pointer and set return value
    fprintf(file, "\tlw $fp, 0($fp)\n");
    fprintf(file, "\tmove $v0, %s\n", mipsRegisters[returnValueReg].regName);
    
    // Return from function
    fprintf(file, "\tjr $ra\n");
    
    // Free all temporary registers
    for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
        if (mipsRegisters[i].isOccupied) {
            mipsRegisters[i].isOccupied = 0;
            MIPS_DEBUG_PRINT("Freed temporary register %s", mipsRegisters[i].regName);
        }
    }
    
    MIPS_DEBUG_PRINT("Function return completed");
}

/* Helper functions for stack operations */
void pushMipsStack(FILE *file, const char* reg) {
    fprintf(file, "\taddi $sp, $sp, -4\n");
    fprintf(file, "\tsw %s, 0($sp)\n", reg);
    MIPS_DEBUG_PRINT("Push %s to stack", reg);
}

void popMipsStack(FILE *file, const char* reg) {
    fprintf(file, "\tlw %s, 0($sp)\n", reg);
    fprintf(file, "\taddi $sp, $sp, 4\n");
    MIPS_DEBUG_PRINT("Pop %s from stack", reg);
}

/* Function call related code generation */
void generateMipsArg(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating code for function arguments");
    
    int argCount = 0;
    // Handle argument pushing
    while (curInterCodes && curInterCodes->code.kind == ARG_InterCode)
    {
        argCount++;
        MIPS_DEBUG_PRINT("Processing argument %d", argCount);
        
        // Get register for argument
        int argReg = allocateMipsRegister(curInterCodes->code.u.singleOP.op, file);
        pushMipsStack(file, mipsRegisters[argReg].regName);
        
        // Free the register
        mipsRegisters[argReg].isOccupied = 0;
        curInterCodes = curInterCodes->next;
    }

    // Now curInterCodes points to the CALL instruction
    if (!curInterCodes || curInterCodes->code.kind != CALL_InterCode) {
        MIPS_DEBUG_PRINT("Error: Expected CALL instruction after ARG");
        return;
    }

    // Get function call information
    Operand resultOp = curInterCodes->code.u.doubleOP.left;   // Return value destination
    Operand funcOp = curInterCodes->code.u.doubleOP.right;    // Function name

    // Call the function
    MIPS_DEBUG_PRINT("Calling function: %s", funcOp->funcName);
    fprintf(file, "\tjal %s\n", funcOp->funcName);

    // Restore stack pointer
    if (argCount > 0) {
        fprintf(file, "\taddi $sp, $sp, %d\n", argCount * 4);
        MIPS_DEBUG_PRINT("Restored stack pointer, removed %d arguments", argCount);
    }

    // Store return value
    int resultReg = allocateMipsRegister(resultOp, file);
    fprintf(file, "\tmove %s, $v0\n", mipsRegisters[resultReg].regName);
    storeMipsRegisterToStack(resultReg, file);
    MIPS_DEBUG_PRINT("Stored return value in %s", mipsRegisters[resultReg].regName);
}

/* I/O related code generation */
void generateMipsRead(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating code for read operation");

    // Save return address
    pushMipsStack(file, "$ra");
    
    // Call read function
    fprintf(file, "\tjal read\n");
    MIPS_DEBUG_PRINT("Called read function");
    
    // Restore return address
    popMipsStack(file, "$ra");

    // Store result
    int resultReg = allocateMipsRegister(curInterCodes->code.u.singleOP.op, file);
    fprintf(file, "\tmove %s, $v0\n", mipsRegisters[resultReg].regName);
    storeMipsRegisterToStack(resultReg, file);
    
    MIPS_DEBUG_PRINT("Stored read result in %s", mipsRegisters[resultReg].regName);
}

void generateMipsWrite(InterCodes curInterCodes, FILE *file)
{
    MIPS_DEBUG_PRINT("Generating code for write operation");

    // Load value to print
    int valueReg = allocateMipsRegister(curInterCodes->code.u.singleOP.op, file);
    fprintf(file, "\tmove $a0, %s\n", mipsRegisters[valueReg].regName);
    MIPS_DEBUG_PRINT("Loaded value to print from %s", mipsRegisters[valueReg].regName);

    // Save return address
    pushMipsStack(file, "$ra");
    
    // Call write function
    fprintf(file, "\tjal write\n");
    MIPS_DEBUG_PRINT("Called write function");
    
    // Restore return address
    popMipsStack(file, "$ra");

    // Free all temporary registers
    for (int i = TEMP_REG_START; i <= TEMP_REG_END; i++) {
        if (mipsRegisters[i].isOccupied) {
            mipsRegisters[i].isOccupied = 0;
            MIPS_DEBUG_PRINT("Freed temporary register %s", mipsRegisters[i].regName);
        }
    }
}