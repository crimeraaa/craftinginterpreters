#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    vm_init(); // Is an internal global scope to `vm.c`.
    LoxChunk chunk;
    chunk_init(&chunk);
    
    /* For now, hand roll the instructions for `OP_CONSTANT`. */
    int constant = chunk_add_constant(&chunk, 1.2);
    chunk_write(&chunk, OP_CONSTANT, 123); // 0: `OP_CONSTANT` itself.
    chunk_write(&chunk, constant, 123); // 1: Index of the constant value.
    chunk_write(&chunk, OP_NEGATE, 123);
    chunk_write(&chunk, OP_RETURN, 123);
    
    chunk_disassemble(&chunk, "test chunk");
    vm_interpret(&chunk);

    vm_free();
    chunk_free(&chunk);
    return 0;
}
