#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

/* Named integer constants for each of our 1-byte instructions. */
typedef enum {
    // Load constant: 1 operand (index into chunk's constant pool)
    OP_CONSTANT, 
    // Keywords, with which we can skip straight to the parser using `literal()`.
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    // III:18.4.1: Equality and comparison operators
    // For simplicity, we *don't* include explicit opcodes for >= and <=.
    // This is to teach about how bytecode compilers don't need to follow the user!
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_UNM,   // Unary operator. Unary negation, a.k.a "Unary Minus": 1 operand.
    // Binary operators
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_RET,   // Return statement. 0 operands.
    OP_NOT,   // III:18.4.1: Logical not and falsiness
    OP_PRINT, // III:21.1.1: Print Statements
    OP_JUMP,  // III:23.1.1: Else Clauses -- unconditional jumps
    OP_JUMP_IF_FALSE, // III:23.1: If Statements
    OP_LOOP,  // III:23.3: While Statements
    OP_POP,   // III:21.1.2: Expression Statements
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL, // III:21.3: Reading Variables
    OP_GET_LOCAL, // III:22.4.1: Interpreting local variables
    OP_SET_GLOBAL, // III:21.4: Assignment
    OP_SET_LOCAL,
} LoxOpCode;

/* Our dynamic array to hold chunks of code. */
typedef struct {
    LoxValueArray constants; // List of constant values for this chunk.
    uint8_t *code; // List of opcodes/instructions to be executed, in order.
    int *lines; // Used only for runtime errors. Parallels the bytecode array.
    int count; // Current number of opcodes/instructions we have.
    int capacity; // Total number of opcodes/instructions we can hold for now.
} LoxChunk;

/* Start off empty. */
void init_chunk(LoxChunk *chunk);

/* Free this chunk's `code` and `constants` sections. */
void free_chunk(LoxChunk *chunk);

/* Append `byte` to this chunk's `code` section. Think a list of opcodes. */
void write_chunk(LoxChunk *chunk, uint8_t byte, int line);

/**
 * @brief   Append `value` to this chunk's constant values pool. In reality,
 *          nothing is stopping you from directly access this chunk's fields!
 *
 * @return  Index of where `value` is located in `chunk->constants.values[]`.
 */
int add_constant(LoxChunk *chunk, LoxValue value);

#endif /* CLOX_CHUNK_H */
