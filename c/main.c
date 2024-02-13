#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    vm_init(); // Is an internal global scope to `vm.c`.
    LoxChunk chunk;
    chunk_init(&chunk);
    
    /** 
     * For now, hand roll the instructions for `OP_CONSTANT`. 
     * III:15.3.1: Run the expression -(1.2 + 3.4 / 5.6)
     */
    int constant = chunk_add_constant(&chunk, 1.2);
    chunk_write(&chunk, OP_CONSTANT, 123); // 0: `OP_CONSTANT` itself.
    chunk_write(&chunk, constant, 123); // 1: Index of the constant value.
                                        
    constant = chunk_add_constant(&chunk, 3.4); // Prep operands for OP_ADD
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, constant, 123);
    
    chunk_write(&chunk, OP_ADD, 123); // 1.2 + 3.4
    
    constant = chunk_add_constant(&chunk, 5.6); // Prep operands for  3.6 / 5.6
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, constant, 123);

    chunk_write(&chunk, OP_DIV, 123); // 3.6 / 5.6
    chunk_write(&chunk, OP_UNM, 123); // -(3.6 / 5.6)
    chunk_write(&chunk, OP_RET, 123);
    
    chunk_disassemble(&chunk, "test chunk");
    vm_interpret(&chunk);

    vm_free();
    chunk_free(&chunk);
    return 0;
}
