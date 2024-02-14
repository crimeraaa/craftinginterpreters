#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

/* Hand-write bytecode for the expression: 1 * 2 + 3 */
void challenge15_1A(int line);

/* Hand-write bytecode for the expression 1 + 2 * 3. */
void challenge15_1B(int line);

/* Hand-write bytecode for the expression 3 - 2 - 1. */
void challenge15_1C(int line);

/* Hand-write bytecode for the expression 1 + 2 * 3 - 4 / -5 */
void challenge15_1D(int line);

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
    // Free the chunk before moving on to the next for now, as we need to clear
    // the allocations!
    chunk_free(&chunk);

    challenge15_1A(__LINE__);
    challenge15_1B(__LINE__);
    challenge15_1C(__LINE__);
    challenge15_1D(__LINE__);

    vm_free();
    return 0;
}

void challenge15_1A(int line)
{
    LoxChunk chunk;
    chunk_init(&chunk);    
    // Set up the expression: 1 * 2, Start with binary operator operands.
    int constant = chunk_add_constant(&chunk, 1);
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    constant = chunk_add_constant(&chunk, 2);
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    // Now that we have the operands, we can evaluate the multiplication.
    chunk_write(&chunk, OP_MUL, line);
    
    // Now we set up the expression x + 3, where x is the result of awhile ago.
    // Since the result of 1 * 2 is already at the top of the stack, we only 
    // need to push the constant 3.
    constant = chunk_add_constant(&chunk, 3);
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    // Now that constant 3 is at the top of the stack, and the result of 1 * 2
    // is right below it, we can use the appropriate binary operator which will
    // automatically pop them off in the correct order.
    chunk_write(&chunk, OP_ADD, line);
    
    // ALWAYS remember this, otherwise the interpreter won't know when to stop!
    chunk_write(&chunk, OP_RET, line);
    
    chunk_disassemble(&chunk, __func__);
    vm_interpret(&chunk);
    chunk_free(&chunk);
}

void challenge15_1B(int line)
{
    LoxChunk chunk;
    chunk_init(&chunk);
    
    // Load all 3 operands, then evaluate multiplication first since 2 and 3 
    // will be on the top of the stack, with 3 on top and 2 right below it.
    // This should replicate PEMDAS/BODMAS operator precedence.
    int constant = chunk_add_constant(&chunk, 1);
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);

    constant = chunk_add_constant(&chunk, 2);
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    constant = chunk_add_constant(&chunk, 3);
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    chunk_write(&chunk, OP_MUL, line); // 2 * 3 = 6 (on top of stack)
    chunk_write(&chunk, OP_ADD, line); // 1 + 6 = 7 (on top of stack)
    chunk_write(&chunk, OP_RET, line);

    chunk_disassemble(&chunk, __func__);
    vm_interpret(&chunk);
    chunk_free(&chunk);    
}

void challenge15_1C(int line)
{
    LoxChunk chunk;
    chunk_init(&chunk);
    
    // 3 - 2 - 1, so due to operator precedence we load the operands for 3 - 2
    // then evaluate that since it's the highest precedence in this expression.
    // I'm thinking this because addition/subtraction are left-associative.
    int constant = chunk_add_constant(&chunk, 3); // load constant 3 (OP_SUB lhs)
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    constant = chunk_add_constant(&chunk, 2); // load constant 2 (OP_SUB rhs)
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    chunk_write(&chunk, OP_SUB, line); // 3 - 2 = 1 (next OP_SUB lhs)

    constant = chunk_add_constant(&chunk, 1); // load constant 1 (nextOP_SUB rhs)
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, constant, line);
    
    chunk_write(&chunk, OP_SUB, line); // 1 - 1 = 0
    chunk_write(&chunk, OP_RET, line);
    
    chunk_disassemble(&chunk, __func__);
    vm_interpret(&chunk);
    chunk_free(&chunk);
}

void challenge15_1D(int line)
{
    // 1 + 2 * 3 - 4 / -5
    LoxChunk chunk;
    chunk_init(&chunk);
    
    // Load 1, 2 and 3, but evaluate 2 * 3 due to operator precedence
    int index = chunk_add_constant(&chunk, 1); // OP_ADD lhs
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, index, line);
    // Stack: [1]

    index = chunk_add_constant(&chunk, 2); // OP_MUL lhs
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, index, line);
    // Stack: [1][2]

    index = chunk_add_constant(&chunk, 3); // OP_MUL rhs
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, index, line);
    // Stack: [1][2][3]
    
    chunk_write(&chunk, OP_MUL, line); // 2 * 3 = 6, OP_ADD rhs
    // Stack: [1][6]

    chunk_write(&chunk, OP_ADD, line); // 1 + 6 (since add/sub is left assoc.)
    // Stack: [7]

    index = chunk_add_constant(&chunk, 4); // OP_DIV lhs
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, index, line);
    // Stack: [7][4]

    // Lox doesn't have negative literals, we do unary negation at runtime.
    index = chunk_add_constant(&chunk, 5); // OP_UNM op, OP_DIV rhs
    chunk_write(&chunk, OP_CONSTANT, line);
    chunk_write(&chunk, index, line);
    // Stack: [7][4][5]

    chunk_write(&chunk, OP_UNM, line);
    // Stack: [7][4][-5]
 
    chunk_write(&chunk, OP_DIV, line); // 4 / -5 = -0.8
    // Stack: [7][-0.8]
    
    chunk_write(&chunk, OP_SUB, line); // 7 - -0.8 = 7.8
    // Stack: [7.8]

    chunk_write(&chunk, OP_RET, line);

    chunk_disassemble(&chunk, __func__);
    vm_interpret(&chunk);
    chunk_free(&chunk);
}
