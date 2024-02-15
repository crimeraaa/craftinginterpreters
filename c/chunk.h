#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

/* Named integer constants for each of our 1-byte instructions. */
typedef enum {
    OP_CONSTANT, // Load constant: 1 operand (index into chunk's constant pool)
    OP_UNM, // Unary operator. Unary negation, a.k.a "Unary Minus": 1 operand.
    OP_ADD, // Binary operator.
    OP_SUB, // Binary operator.
    OP_MUL, // Binary operator.
    OP_DIV, // Binary operator.
    OP_RET, // Return statement. 0 operands.
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
