#ifndef __OBJECT_CODE_H__
#define __OBJECT_CODE_H__

#include "tools.h"

// Debug configuration
#define MIPS_DEBUG 0  // Set to 1 to enable debug output
#define DEBUG_PREFIX "[MIPS] "

// Debug print macro
#define MIPS_DEBUG_PRINT(fmt, ...) \
    do { if (MIPS_DEBUG) fprintf(stderr, DEBUG_PREFIX fmt "\n", ##__VA_ARGS__); } while(0)

// Register management constants
#define TEMP_REG_START 8
#define TEMP_REG_END 15
#define MAX_NAME_LENGTH 32

// External declarations
extern InterCodes interCodeListHead;

// Type definitions
typedef struct MipsRegisterAllocation_ *MipsRegisterAllocation;

// Register structure definition
typedef struct MipsRegister {
    bool isOccupied;                    // Whether register is in use
    char *regName;                      // Register name (e.g., "$t0")
    MipsRegisterAllocation varAlloc;    // Variable allocation info for memory operations
} MipsRegister;

// Variable allocation structure definition
typedef struct MipsRegisterAllocation_ {
    char name[MAX_NAME_LENGTH];         // Variable name
    int regNum;                         // Register number
    int stackOffset;                    // Stack frame offset
    MipsRegisterAllocation next;        // Next allocation in list
} MipsRegisterAllocation_;

// Register management functions
void initMipsRegisters(void);
int allocateMipsRegister(Operand op, FILE *file);
void storeMipsRegisterToStack(int regIndex, FILE *file);
MipsRegisterAllocation getMipsVarAllocation(Operand op);
void createMipsVarAllocation(Operand op);

// Stack operation helpers
void pushMipsStack(FILE *file, const char* reg);
void popMipsStack(FILE *file, const char* reg);

// Main code generation function
void generateMipsCode(FILE *file);

// Instruction-specific code generation functions
void generateMipsFunction(InterCodes curInterCodes, FILE *file);
void generateMipsAssignment(InterCodes curInterCodes, FILE *file);
void generateMipsArithmetic(char *opType, InterCodes curInterCodes, FILE *file);
void generateMipsAdd(InterCodes curInterCodes, FILE *file);
void generateMipsSub(InterCodes curInterCodes, FILE *file);
void generateMipsMul(InterCodes curInterCodes, FILE *file);
void generateMipsDiv(InterCodes curInterCodes, FILE *file);

// Control flow code generation
void generateMipsGoto(InterCodes curInterCodes, FILE *file);
void generateMipsIfGoto(InterCodes curInterCodes, FILE *file);
void generateMipsReturn(InterCodes curInterCodes, FILE *file);

// Function call related code generation
void generateMipsArg(InterCodes curInterCodes, FILE *file);

// I/O related code generation
void generateMipsRead(InterCodes curInterCodes, FILE *file);
void generateMipsWrite(InterCodes curInterCodes, FILE *file);

#endif // __OBJECT_CODE_H__