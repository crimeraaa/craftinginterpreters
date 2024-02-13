#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT, // Load a constant for use. 1 operand: a constant index.
    OP_RETURN, // 0 operands.
} LoxOpCode; // Named integer constants for each of our 1-byte instructions.

/* Our dynamic array to hold chunks of code. */
typedef struct {
    LoxValueArray constants; // List of constant values for this chunk.
    uint8_t *code; // List of opcodes/instructions.
    int *lines; // Used only for runtime errors. Parallels the bytecode array.
    int count; // Current number of opcodes/instructions we have.
    int capacity; // Total number of opcodes/instructions we can hold at most.
} LoxChunk; 

/* Start off empty. */
void lox_init_chunk(LoxChunk *chunk);

/* Free this chunk's `code` and `constants` sections. */
void lox_free_chunk(LoxChunk *chunk);

/* Append `opcode` to this chunk's `code` section. Think a list of opcodes. */
void lox_write_chunk(LoxChunk *chunk, uint8_t opcode, int line);

/** 
 * @brief   Append `value` to this chunk's constant values pool. In reality, 
 *          nothing is stopping you from directly access this chunk's fields!
 * 
 * @return  Index of where `value` is located in `chunk->constants.values[]`.
 */
int lox_add_constant(LoxChunk *chunk, LoxValue value);


#endif /* CLOX_CHUNK_H */
