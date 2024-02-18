#include <stdio.h>

#include "debug.h"

void disassemble_chunk(LoxChunk *chunk, const char *name) {
    printf("== %s ==\n", name); // Indicate *which* chunk we're looking at

    // We rely on the disassembly to increment `offset` for us.
    // This is because instructions can have different sizes.
    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

static int simple_instruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1; // Return start of next instruction
}

static int byte_instruction(const char *name, LoxChunk *chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

/**
 * III:23.1.1
 * 
 * We read 2 8-bit values and use them to build 1 16-bit value.
 * 
 * 1. code[offset + 1]: This is an 8-bit value representing the upper 8 bits of
 *                      a full 16-bit value. We bitshift it left by 8 bits to
 *                      retrieve back that 16-bit representation of them.
 *
 * 2. code[offset + 2]: This is an 8-bit value representing the lower 8 bits of
 *                      a full 16-bit value. We can bitwise OR it so that the all
 *                      the 1 bits of both operands are combined into a 16-bit.
 */
#define read_jump(code, offset) (code[offset + 1] << 8) | code[offset + 2]

/**
 * III:23.1.1
 * 
 * Jump instructions takes 3 bytes in total. 1 for the jump instruction itself
 * and 2 bytes for the operand.
 */
static int jump_instruction(const char *name, int sign, LoxChunk *chunk, int offset) {
    uint16_t jump = (uint16_t)read_jump(chunk->code, offset);
    printf("%-16s %4d -> %i\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int constant_instruction(const char *name, LoxChunk *chunk, int offset) {
    // code[offset] is OP_CONSTANT opcode, code[offset + 1] is the operand.
    // This gives us an index into this chunk's constants pool.
    uint8_t index = chunk->code[offset + 1]; 
    printf("%-16s %4d '", name, index);
    print_value(chunk->constants.values[index]);
    printf("'\n");
    return offset + 2; // `OP_CONSTANT`takes 2 bytes as it has 1 operand.
}

int disassemble_instruction(LoxChunk *chunk, int offset)
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
    case OP_CONSTANT: return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_DEFINE_GLOBAL: return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL: return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_GET_LOCAL: return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_GLOBAL: return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_SET_LOCAL: return byte_instruction("OP_SET_LOCAL", chunk, offset);
    case OP_NIL: return simple_instruction("OP_NIL", offset);
    case OP_TRUE: return simple_instruction("OP_TRUE", offset);
    case OP_FALSE: return simple_instruction("OP_FALSE", offset);
    case OP_EQUAL: return simple_instruction("OP_EQUAL", offset);
    case OP_GREATER: return simple_instruction("OP_GREATER", offset);
    case OP_LESS: return simple_instruction("OP_LESS", offset);
    case OP_ADD: return simple_instruction("OP_ADD", offset);
    case OP_SUB: return simple_instruction("OP_SUB", offset);
    case OP_MUL: return simple_instruction("OP_MUL", offset);
    case OP_DIV: return simple_instruction("OP_DIV", offset);
    case OP_NOT: return simple_instruction("OP_NOT", offset);
    case OP_UNM: return simple_instruction("OP_UNM", offset);
    case OP_PRINT: return simple_instruction("OP_PRINT", offset);
    case OP_POP: return simple_instruction("OP_POP", offset);
    case OP_JUMP: return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE: return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_RET: return simple_instruction("OP_RET", offset);
    default:
        printf("Unknown opcode %d.\n", opcode);
        return offset + 1; // Indicate the likely start of the next instruction.
    }
}

#undef read_jump
