#ifndef CLOXDEBUG_H
#define CLOXDEBUG_H

#include "chunk.h"

/**
 * Write out the equivalent opcodes for each instruction in `chunk`.
 */
void disassemble_chunk(LoxChunk *chunk, const char *name);

/**
 * Actually determines the particular opcode for a particular offset into `chunk`.
 */
int disassemble_instruction(LoxChunk *chunk, int offset);

#endif /* CLOXDEBUG_H */
