#ifndef CLOXDEBUG_H
#define CLOXDEBUG_H

#include "chunk.h"

/**
 * Write out the equivalent opcodes for each instruction in `chunk`.
 */
void chunk_disassemble(LoxChunk *chunk, const char *name);

/**
 * Actually determines the particular opcode for a particular offset into `chunk`.
 */
int chunk_disassemble_instruction(LoxChunk *chunk, int offset);

#endif /* CLOXDEBUG_H */
