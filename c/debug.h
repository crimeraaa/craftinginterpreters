#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"

/**
 * Write out the equivalent opcodes for each instruction in `chunk`.
 */
void lox_disassemble_chunk(LoxChunk *chunk, const char *name);

/**
 * Actually determines the particular opcode for a particular offset into `chunk`.
 */
int lox_disassemble_instruction(LoxChunk *chunk, int offset);

#endif /* CLOX_DEBUG_H */
