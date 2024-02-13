#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, char *argv[]) {
    LoxChunk chunk;
    lox_init_chunk(&chunk);
    
    /* For now, hand roll the instructions for `OP_CONSTANT`. */
    int constant = lox_add_constant(&chunk, 1.2);
    lox_write_chunk(&chunk, OP_CONSTANT, 123); // 0: `OP_CONSTANT` itself.
    lox_write_chunk(&chunk, constant, 123); // 1: Index of the constant value.

    lox_write_chunk(&chunk, OP_RETURN, 123);
    
    lox_disassemble_chunk(&chunk, "test chunk");
    lox_free_chunk(&chunk);
    return 0;
}
