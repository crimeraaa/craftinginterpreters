#include <stdio.h>

#include "debug.h"

void chunk_disassemble(LoxChunk *chunk, const char *name)
{
    printf("== %s ==\n", name); // Indicate *which* chunk we're looking at

    // We rely on the disassembly to increment `offset` for us.
    // This is because instructions can have different sizes.
    for (int offset = 0; offset < chunk->count;) {
        offset = chunk_disassemble_instruction(chunk, offset);
    }
}

static int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1; // Return start of next instruction
}

static int constant_instruction(const char *name, LoxChunk *chunk, int offset)
{
    // code[offset] is OP_CONSTANT opcode, code[offset + 1] is the operand.
    // This gives us an index into this chunk's constants pool.
    uint8_t index = chunk->code[offset + 1]; 
    printf("%-16s %4d '", name, index);
    value_print(chunk->constants.values[index]);
    printf("'\n");
    return offset + 2; // `OP_CONSTANT`takes 2 bytes as it has 1 operand.
}

int chunk_disassemble_instruction(LoxChunk *chunk, int offset)
{
    printf("%04i ", offset); // Where in the chunk is this instruction?
    // Separate instructions that come from the same source line with '|'.
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
    uint8_t opcode = chunk->code[offset]; // a.k.a our opcode
    switch(opcode) {
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_ADD:
        return simple_instruction("OP_ADD", offset);
    case OP_SUB:
        return simple_instruction("OP_SUB", offset);
    case OP_MUL:
        return simple_instruction("OP_MUL", offset);
    case OP_DIV:
        return simple_instruction("OP_DIV", offset);
    case OP_UNM:
        return simple_instruction("OP_UNM", offset);
    case OP_RET:
        return simple_instruction("OP_RET", offset);
    default:
        printf("Unknown opcode %d.\n", opcode);
        return offset + 1; // Indicate the likely start of the next instruction.
    }
}


