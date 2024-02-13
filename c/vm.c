#include "vm.h"
#include "debug.h"

LoxVM vm; // For simplicity we'll use global state. Not good practice!

/** 
 * Make the stacktop pointer point to the stack base. 
 * Note that because our stack is, well, stack-allocated, simple moving the
 * pointer back is more than enough to indicate which memory is used.
 */
static void vm_reset_stack() 
{
    vm.stacktop = vm.stack;
}

void vm_init(void)
{
    vm_reset_stack();
}

void vm_free(void)
{
    
}

void vm_push(LoxValue value)
{
    *vm.stacktop = value;
    vm.stacktop++;
}

LoxValue vm_pop(void)
{
    vm.stacktop--;
    return *vm.stacktop;
}

/* No matter the precedence, the original value of `*vm.ip` is returned. */
#define VM_READ_BYTE()      (*vm.ip++)
#define VM_READ_CONSTANT()  (vm.chunk->constants.values[VM_READ_BYTE()])
/** 
 * Ugly, but it does ensure *some* type safety! 
 * Also remember that `rhs` is the latest element, hence we pop it off first.
 */
#define VM_BINARY_OP(op)    \
    do { \
        double rhs = vm_pop(); \
        double lhs = vm_pop(); \
        vm_push(lhs op rhs); \
    } while(false)

static LoxInterpretResult vm_run(void)
{
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        // Show the current contents of the stack before interpreting opcodes.
        // We go from the bottom of the stack all the way to the top/latest.
        printf("             ");
        for (LoxValue *slot = vm.stack; slot < vm.stacktop; slot++) {
            printf("[ ");
            value_print(*slot);
            printf(" ]");
        }
        printf("\n");
        // Get relative offset from the beginning of the bytecode.
        // Remember: vm.ip points to next opcode and vm.chunk->code is start.
        chunk_disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t opcode;
        switch (opcode = VM_READ_BYTE()) {
        case OP_CONSTANT: {
            LoxValue constant = VM_READ_CONSTANT();
            vm_push(constant); // "Produce" a value by pushing it onto the stack.
            break;
        }
        case OP_ADD:    { VM_BINARY_OP(+); break; }
        case OP_SUB:    { VM_BINARY_OP(-); break; }
        case OP_MUL:    { VM_BINARY_OP(*); break; }
        case OP_DIV:    { VM_BINARY_OP(/); break; }
        // Seems silly but think about it for a bit!
        case OP_UNM: { 
            vm_push(-vm_pop()); 
            break; 
        }
        case OP_RET: { value_print(vm_pop());
            printf("\n");
            return INTERPRET_OK;
        }}
    }
}

#undef VM_BINARY_OP
#undef VM_READ_BYTE
#undef VM_READ_CONSTANT

LoxInterpretResult vm_interpret(LoxChunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return vm_run();
}
