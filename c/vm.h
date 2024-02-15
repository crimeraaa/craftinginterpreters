#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "common.h"
#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

/* Say hello to the CLox Virtual Machine! It executes Lox bytecode. */
typedef struct {
    LoxChunk *chunk; // Chunk of bytecode we need to execute.
    uint8_t *ip; // Instruction pointer, always points to the *next* instruction.
    LoxValue stack[STACK_MAX]; // Holds all our globals and locals.
    LoxValue *stacktop; // Points to 1 past the stack's latest (top) element.
} LoxVM;

/* Used by `LoxVM` to report and address various error states. */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} LoxInterpretResult;

void init_vm(void);
void free_vm(void);

/* Run the internal CLox Virtual machine given a line of code or a script. */
LoxInterpretResult interpret_vm(const char *source);

/* Push some value to the virtual machine's stack. */
void push_vm(LoxValue value);

/* Pop some value off of the virtual machine's stack. */
LoxValue pop_vm(void);

#endif /* CLOX_VM_H */
