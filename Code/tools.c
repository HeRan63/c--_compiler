#include <stdio.h>
#include "tools.h"
#include "semantictool.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Global variables for intermediate code generation
int varNo = 0;
int tempNo = 0;
int labelNo = 0;
InterCodes interCodeListHead = NULL;
InterCodes interCodeListTail = NULL;

/* Implementation of AST node creation */
ASTNode* ast_create_node(const char* name, const char* value, ASTNodeType type, int lineno) {
	ASTNode* newNode = (ASTNode*)malloc(sizeof(ASTNode));
	if (!newNode) return NULL;
	
	newNode->type = type;
	newNode->lineno = lineno;
	
	/* Allocate memory for name and value strings */
	size_t nameLen = strlen(name) + 1;
	size_t valueLen = strlen(value) + 1;
	
	newNode->name = (char*)malloc(nameLen * sizeof(char));
	newNode->value = (char*)malloc(valueLen * sizeof(char));
	
	/* Copy strings */
	memcpy(newNode->name, name, nameLen);
	memcpy(newNode->value, value, valueLen);
	
	/* Initialize pointers */
	newNode->firstChild = NULL;
	newNode->nextSibling = NULL;
	
	return newNode;
}

/* Add children to an AST node */
void ast_add_child(ASTNode* parent, int num_children, ...) {
	/* Check for null parent */
	if (!parent) return;
	
	va_list childrenList;
	va_start(childrenList, num_children);
	
	/* Process each child */
	for (int i = 0; i < num_children; i++) {
		ASTNode* childNode = va_arg(childrenList, ASTNode*);
		
		if (childNode) {
			/* Link child to parent */
			childNode->nextSibling = parent->firstChild;
			parent->firstChild = childNode;
			
			/* Update parent line number */
			parent->lineno = childNode->lineno;
		}
	}
	
	va_end(childrenList);
}

/* Recursively free memory for AST tree */
void ast_destroy(ASTNode* root) {
	if (!root) return;
	
	/* Recursive destruction of children and siblings */
	ast_destroy(root->firstChild);
	ast_destroy(root->nextSibling);
	
	/* Free node data */
	free(root->name);
	free(root->value);
	free(root);
}

/* Fetch a specific child node by position */
ASTNode* getChild(ASTNode* parentNode, int position) {
	if (!parentNode) return NULL;
	
	ASTNode* currentChild = parentNode->firstChild;
	int currentPos = 0;
	
	/* Traverse to the requested position */
	while (currentChild && currentPos < position) {
		currentChild = currentChild->nextSibling;
		currentPos++;
	}
	
	return currentChild;
}

/* Compare two strings for equality */
bool stringComparison(char* str1, char* str2) {
    if (!str1 || !str2) {
        printf("WARNING: stringComparison接收到NULL参数: str1=%p, str2=%p\n", 
               (void*)str1, (void*)str2);
        return false;
    }
    
    bool result = (strcmp(str1, str2) == 0);
   // printf("DEBUG: 字符串比较: '%s' vs '%s', 结果=%d\n", str1, str2, result ? 1 : 0);
    return result;
}

/* Convert octal string to integer */
int to_Oct(char* octalStr) {
    int result = 0;
	
	/* Skip the leading '0' */
	octalStr++;
	
	/* Process each digit */
	while (*octalStr) {
		result = (result << 3) | (*octalStr - '0');
		octalStr++;
	}
	
    return result;
}

/* Convert hexadecimal string to integer */
int to_Hex(char* hexStr) {
    int result = 0;
	
	/* Skip "0x" or "0X" prefix */
	hexStr += 2;
	
	/* Process each hex digit */
	while (*hexStr) {
		if (*hexStr >= '0' && *hexStr <= '9') {
			result = (result << 4) | (*hexStr - '0');
		} else if (*hexStr >= 'a' && *hexStr <= 'f') {
			result = (result << 4) | (*hexStr - 'a' + 10);
		} else if (*hexStr >= 'A' && *hexStr <= 'F') {
			result = (result << 4) | (*hexStr - 'A' + 10);
		}
		hexStr++;
	}
	
    return result;
}
/* My_itoa 自定义itoa */
char* ita(int num, char *str)
{
    // Handle negative numbers
    int is_negative = (num < 0);
    if (is_negative) {
        num = -num;
        *str++ = '-';
    }
    
    // Convert number to string in reverse order
    char *start = str;
    do {
        *str++ = '0' + (num % 10);
        num /= 10;
    } while (num);
    *str = '\0';
    
    // Reverse the string
    char *end = str - 1;
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}
/* Convert decimal string to integer */
int to_Dec(char* decStr) {
	int result = 0;
	int sign = 1;
	
	/* Handle negative numbers */
	if (*decStr == '-') {
		sign = -1;
		decStr++;
	}
	
	/* Process each digit */
	while (*decStr >= '0' && *decStr <= '9') {
		result = result * 10 + (*decStr - '0');
		decStr++;
	}
	
	return result * sign;
}

/* Detect number format and convert accordingly */
int My_atoi(char* str) {
	/* Check format by prefix */
	if (str[0] == '0') {
		if (str[1] == 'x' || str[1] == 'X') {
            return to_Hex(str);
		} else {
            return to_Oct(str);
		}
	} else {
        return to_Dec(str);
    }
}

/* Symbol table hash function (PJW algorithm) */
unsigned int hash_pjw(char* symbolName) {
	unsigned int hashVal = 0;
	unsigned int g;
	
	/* Calculate hash value */
	while (*symbolName) {
		hashVal = (hashVal << 2) + *symbolName;
		
		/* Apply bitmask for size constraints */
		if ((g = hashVal & ~TABLESIZE)) {
			hashVal = (hashVal ^ (g >> 12)) & TABLESIZE;
		}
		
		symbolName++;
	}
	
	return hashVal;
}

/* Print AST node information */
void print_node_info(const char* name, const char* value) {
    printf("%s", name);
    if (value != NULL && strlen(value) > 0) {
        printf(": %s", value);
    }
    printf("\n");
}

/* Print AST tree with indentation */
void ast_print(ASTNode* root, int depth) {
    if (root == NULL) return;
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    // Print current node
    print_node_info(root->name, root->value);
    
    // Recursively print children
    ASTNode* child = root->firstChild;
    while (child != NULL) {
        ast_print(child, depth + 1);
        child = child->nextSibling;
    }
}

// ------------------------ INTERMEDIATE CODE IMPLEMENTATION -----------------------

/**
 * Allocates and configures an intermediate code node based on operation type
 * @param opType Operation type identifier
 * @param ... Variable arguments depending on operation type
 */
void ir_generate_code(int opType, ...) {
    // Initialize argument processing
    va_list argList;
    va_start(argList, opType);
    
    // Allocate memory for the new node
    InterCodes icNode = (InterCodes)malloc(sizeof(struct InterCodes_));
    if (!icNode) {
        printf("Memory allocation error in ir_generate_code\n");
        va_end(argList);
        return;
    }
    
    // Set operation type and initialize pointers
    icNode->code.kind = opType;
    icNode->next = NULL;
    
    // Process arguments based on operation type
    switch (opType) {
        // Single operand operations
        case LABEL_InterCode:   // Label definition
        case FUNC_InterCode:    // Function definition
        case GOTO_InterCode:    // Unconditional jump
        case RETURN_InterCode:  // Function return
        case ARG_InterCode:     // Function argument
        case PARAM_InterCode:   // Parameter declaration
        case READ_InterCode:    // Read from console
        case WRITE_InterCode:   // Write to console
            icNode->code.u.singleOP.op = va_arg(argList, Operand);
            break;
            
        // Two operand operations
        case ASSIGN_InterCode:      // Assignment
        case GET_ADDR_InterCode:    // Address-of operator
        case GET_CONTENT_InterCode: // Dereference operator
        case TO_ADDR_InterCode:     // Store to address
        case CALL_InterCode:        // Function call
        case DEC_InterCode:         // Memory allocation
            icNode->code.u.doubleOP.left = va_arg(argList, Operand);
            icNode->code.u.doubleOP.right = va_arg(argList, Operand);
            break;
            
        // Three operand operations (arithmetic)
        case ADD_InterCode:  // Addition
        case SUB_InterCode:  // Subtraction
        case MUL_InterCode:  // Multiplication
        case DIV_InterCode:  // Division
            icNode->code.u.tripleOP.result = va_arg(argList, Operand);
            icNode->code.u.tripleOP.op1 = va_arg(argList, Operand);
            icNode->code.u.tripleOP.op2 = va_arg(argList, Operand);
            break;
            
        // Conditional branch operation
        case IFGOTO_InterCode:
            icNode->code.u.ifgotoOP.op1 = va_arg(argList, Operand);
            icNode->code.u.ifgotoOP.relop = va_arg(argList, char*);
            icNode->code.u.ifgotoOP.op2 = va_arg(argList, Operand);
            icNode->code.u.ifgotoOP.label = va_arg(argList, Operand);
            break;
            
        default:
            printf("Unknown operation type: %d\n", opType);
            free(icNode);
            va_end(argList);
            return;
    }
    
    // Add the node to the doubly-linked list
     if (!icNode) return;
    
    // Connect to surrounding nodes in the circular list
    icNode->prev = interCodeListTail;
    icNode->next = interCodeListHead;
    
    // Update adjacent nodes' references
    if (interCodeListTail) interCodeListTail->next = icNode;
    if (interCodeListHead) interCodeListHead->prev = icNode;
    
    // Update tail pointer
    interCodeListTail = icNode;
    va_end(argList);
}

/**
 * Creates and initializes a new operand object
 * @param operandKind Type of operand to create
 * @param dataType The data type of the operand (value or address)
 * @param ... Additional parameters based on operand type
 * @return The newly created operand
 */
Operand ir_create_operand(int operandKind, int dataType, ...) {
    // Process variable arguments
    va_list args;
    va_start(args, dataType);
    
    // Allocate memory for the operand
    Operand op = (Operand)malloc(sizeof(struct Operand_));
    if (!op) {
        printf("Memory allocation error in ir_create_operand\n");
        va_end(args);
        return NULL;
    }
    
    // Initialize general properties
    op->kind = operandKind;
    op->type = dataType;
    
    // Set specific properties based on operand kind
    switch (operandKind) {
        case VARIABLE_OP:  // Variable operand
            op->var_no = varNo++;  // Assign and increment variable number
            op->varName = va_arg(args, char*);
            break;
            
        case CONSTANT_OP:  // Constant operand
            op->value = va_arg(args, int);
            break;
            
        case TEMP_OP:  // Temporary variable
            op->var_no = tempNo++;  // Assign and increment temporary variable number
            break;
            
        case FUNCTION_OP:  // Function name
            op->funcName = va_arg(args, char*);
            break;
            
        case LABEL_OP:  // Label for code jumps
            op->var_no = labelNo++;  // Assign and increment label number
            break;
            
        default:
            printf("Unknown operand kind: %d\n", operandKind);
            free(op);
            va_end(args);
            return NULL;
    }
    
    va_end(args);
    return op;
}

/**
 * Outputs the string representation of an operand to the given file
 * @param op The operand to print
 * @param outFile The file to write output to
 */
void ir_output_operand(Operand op, FILE* outFile) {
    // Validate input parameters
    if (!op) {
        fprintf(outFile, "[NULL_OPERAND]");
        return;
    }
    
    if (!outFile) {
        printf("Error: Invalid output file in ir_output_operand\n");
        return;
    }
    
    // Format and output based on operand type
    switch (op->kind) {
        case VARIABLE_OP:  // Variable
            fprintf(outFile, "%sv%d", (op->type == ADDRESS ? "&" : ""), op->var_no);
            break;
            
        case CONSTANT_OP:  // Constant value
            fprintf(outFile, "#%d", op->value);
            break;
            
        case TEMP_OP:  // Temporary variable
            fprintf(outFile, "%st%d", (op->type == ADDRESS ? "*" : ""), op->var_no);
            break;
            
        case LABEL_OP:  // Code label
            fprintf(outFile, "%d", op->var_no);
            break;
            
        case FUNCTION_OP:  // Function name
            if (op->funcName) {
                fprintf(outFile, "%s", op->funcName);
            } else {
                fprintf(outFile, "[UNNAMED_FUNCTION]");
            }
            break;
            
        default:
            fprintf(outFile, "[UNKNOWN_OPERAND_TYPE]");
    }
}

/**
 * Writes all intermediate code to the specified output file
 * @param outFile File handle to write the output to
 */
void ir_write_codes(FILE* outFile) {
    // Validate output file
    if (!outFile) {
        printf("Error: Invalid output file in ir_write_codes\n");
        return;
    }
    
    // Check if we have a valid list
    if (!interCodeListHead) {
        printf("Error: Intermediate code list not initialized\n");
        return;
    }
    
    // Check if the list is empty
    if (!interCodeListHead->next || interCodeListHead->next == interCodeListHead) {
        printf("Info: No intermediate code to print\n");
        return;
    }
    
    // Traverse the list and print each code
    InterCodes current = interCodeListHead->next;
    while (current && current != interCodeListHead) {
        // Format output based on code type
        switch (current->code.kind) {
            case LABEL_InterCode:  // Label definition
                fprintf(outFile, "LABEL label");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, " : \n");
                break;
                
            case FUNC_InterCode:  // Function definition
                fprintf(outFile, "FUNCTION ");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, " : \n");
                break;
                
            case ASSIGN_InterCode:  // Assignment
                ir_output_operand(current->code.u.doubleOP.left, outFile);
                fprintf(outFile, " := ");
                ir_output_operand(current->code.u.doubleOP.right, outFile);
                fprintf(outFile, "\n");
                break;
                
            case ADD_InterCode:  // Addition
                ir_output_operand(current->code.u.tripleOP.result, outFile);
                fprintf(outFile, " := ");
                ir_output_operand(current->code.u.tripleOP.op1, outFile);
                fprintf(outFile, " + ");
                ir_output_operand(current->code.u.tripleOP.op2, outFile);
                fprintf(outFile, "\n");
                break;
                
            case SUB_InterCode:  // Subtraction
                ir_output_operand(current->code.u.tripleOP.result, outFile);
                fprintf(outFile, " := ");
                ir_output_operand(current->code.u.tripleOP.op1, outFile);
                fprintf(outFile, " - ");
                ir_output_operand(current->code.u.tripleOP.op2, outFile);
                fprintf(outFile, "\n");
                break;
                
            case MUL_InterCode:  // Multiplication
                ir_output_operand(current->code.u.tripleOP.result, outFile);
                fprintf(outFile, " := ");
                ir_output_operand(current->code.u.tripleOP.op1, outFile);
                fprintf(outFile, " * ");
                ir_output_operand(current->code.u.tripleOP.op2, outFile);
                fprintf(outFile, "\n");
                break;
                
            case DIV_InterCode:  // Division
                ir_output_operand(current->code.u.tripleOP.result, outFile);
                fprintf(outFile, " := ");
                ir_output_operand(current->code.u.tripleOP.op1, outFile);
                fprintf(outFile, " / ");
                ir_output_operand(current->code.u.tripleOP.op2, outFile);
                fprintf(outFile, "\n");
                break;
                
            case GET_ADDR_InterCode:  // Get address
                ir_output_operand(current->code.u.doubleOP.left, outFile);
                fprintf(outFile, " := &");
                ir_output_operand(current->code.u.doubleOP.right, outFile);
                fprintf(outFile, "\n");
                break;
                
            case GET_CONTENT_InterCode:  // Dereference
                ir_output_operand(current->code.u.doubleOP.left, outFile);
                fprintf(outFile, " := *");
                ir_output_operand(current->code.u.doubleOP.right, outFile);
                fprintf(outFile, "\n");
                break;
                
            case TO_ADDR_InterCode:  // Store to address
                fprintf(outFile, "*");
                ir_output_operand(current->code.u.doubleOP.left, outFile);
                fprintf(outFile, " := ");
                ir_output_operand(current->code.u.doubleOP.right, outFile);
                fprintf(outFile, "\n");
                break;
                
            case GOTO_InterCode:  // Unconditional jump
                fprintf(outFile, "GOTO label");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, "\n");
                break;
                
            case IFGOTO_InterCode:  // Conditional branch
                fprintf(outFile, "IF ");
                ir_output_operand(current->code.u.ifgotoOP.op1, outFile);
                fprintf(outFile, " %s ", current->code.u.ifgotoOP.relop);
                ir_output_operand(current->code.u.ifgotoOP.op2, outFile);
                fprintf(outFile, " GOTO label");
                ir_output_operand(current->code.u.ifgotoOP.label, outFile);
                fprintf(outFile, "\n");
                break;
                
            case RETURN_InterCode:  // Function return
                fprintf(outFile, "RETURN ");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, "\n");
                break;
                
            case DEC_InterCode:  // Memory allocation
                fprintf(outFile, "DEC ");
                ir_output_operand(current->code.u.doubleOP.left, outFile);
                fprintf(outFile, " %d", current->code.u.doubleOP.right->value);
                fprintf(outFile, "\n");
                break;
                
            case ARG_InterCode:  // Function argument
                fprintf(outFile, "ARG ");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, "\n");
                break;
                
            case CALL_InterCode:  // Function call
                ir_output_operand(current->code.u.doubleOP.left, outFile);
                fprintf(outFile, " := CALL ");
                ir_output_operand(current->code.u.doubleOP.right, outFile);
                fprintf(outFile, "\n");
                break;
                
            case PARAM_InterCode:  // Parameter declaration
                fprintf(outFile, "PARAM ");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, "\n");
                break;
                
            case READ_InterCode:  // Read from console
                fprintf(outFile, "READ ");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, "\n");
                break;
                
            case WRITE_InterCode:  // Write to console
                fprintf(outFile, "WRITE ");
                ir_output_operand(current->code.u.singleOP.op, outFile);
                fprintf(outFile, "\n");
                break;
                
            default:
                fprintf(outFile, "[UNKNOWN_CODE_TYPE: %d]\n", current->code.kind);
        }
        
        // Move to next code in the list
        current = current->next;
    }
}

/**
 * Creates a deep copy of an operand
 * @param src Source operand to copy
 * @return Newly allocated copy of the operand
 */
Operand ir_duplicate_operand(Operand src) {
    if (!src) return NULL;
    
    // Allocate memory for the copy
    Operand copy = (Operand)malloc(sizeof(struct Operand_));
    if (!copy) {
        printf("Memory allocation error in ir_duplicate_operand\n");
        return NULL;
    }
    
    // Copy all fields
    copy->kind = src->kind;
    copy->type = src->type;
    copy->var_no = src->var_no;
    copy->value = src->value;
    copy->varName = src->varName;
    copy->funcName = src->funcName;
    copy->depth = src->depth;
    
    return copy;
}



/**
 * Calculates the size in bytes for a given type
 * @param type The type to calculate size for
 * @return Size in bytes
 */
int ir_calc_type_size(Type type) {
    if (!type) return 0;
    
    switch (type->kind) {
        case BASIC:  // Basic types (int, float)
            return 4;  // Standard size for basic types
            
        case ARRAY: {  // Array types
            // Compute total size as product of dimensions
            int totalSize = 1;
            Type elementType = type;
            
            // Traverse array dimensions
            while (elementType && elementType->kind == ARRAY) {
                totalSize *= elementType->u.array.size;
                elementType = elementType->u.array.element;
            }
            
            // Multiply by base element size
            if (elementType) {
                totalSize *= ir_calc_type_size(elementType);
            }
            
            return totalSize;
        }
            
        case STRUCTURE: {  // Structure types
            // Sum sizes of all fields
            int structSize = 0;
            FieldList field = type->u.structure.structures;
            
            // Traverse fields
            while (field) {
                structSize += ir_calc_type_size(field->type);
                field = field->nextFieldList;
            }
            
            return structSize;
        }
            
        default:
            printf("Unknown type kind: %d\n", type->kind);
            return 0;
    }
} 