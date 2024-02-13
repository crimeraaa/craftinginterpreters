#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "common.h"
#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

/* Say hello to the CLox Virtual Machine! It executes Lox bytecode. */
typedef struct {
    LoxChunk *chunk; // Chunk we need to execute.
    uint8_t *ip; // Instruction pointer, always points to the *next* instruction.
    LoxValue stack[STACK_MAX];
    LoxValue *stacktop; // Points to 1 past the stack's latest (top) element.
} LoxVM;

/* Used by `LoxVM` to report and address various error states. */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} LoxInterpretResult;

void vm_init(void);
void vm_free(void);
LoxInterpretResult vm_interpret(LoxChunk *chunk);
void vm_push(LoxValue value);
LoxValue vm_pop(void);

#endif /* CLOX_VM_H */
