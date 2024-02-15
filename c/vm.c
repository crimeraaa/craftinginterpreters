#include "vm.h"
#include "compiler.h"
#include "debug.h"

LoxVM vm = {0}; // For simplicity we'll use global state. Not good practice!

/**
 * Make the stacktop pointer point to the stack base.
 * Note that because our stack is, well, stack-allocated, simple moving the
 * pointer back is more than enough to indicate which memory is used.
 */
static void vm_reset_stack()
{
    vm.stacktop = vm.stack;
}

void init_vm(void)
{
    vm_reset_stack();
}

void free_vm(void)
{

}

void push_vm(LoxValue value)
{
    *vm.stacktop = value;
    vm.stacktop++;
}

LoxValue pop_vm(void)
{
    vm.stacktop--;
    return *vm.stacktop;
}

/* Return the current value of `vm.ip` but increment the pointer afterwards. */
#define VM_READ_BYTE()      (*vm.ip++)
#define VM_READ_CONSTANT()  (vm.chunk->constants.values[VM_READ_BYTE()])
/**
 * Ugly, but it does ensure *some* type safety!
 * Note we use a do-while(false) loop to ensure correct scoping while only
 * running the contents once. It also allows/requires users to append a ';'.
 *
 * Also remember that `rhs` is the latest element, hence we pop it off first.
 */
#define VM_BINARY_OP(op)    \
    do { \
        double rhs = pop_vm(); \
        double lhs = pop_vm(); \
        push_vm(lhs op rhs); \
    } while(false)

static LoxInterpretResult run_vm(void)
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
        // Remember: vm.ip points to next byte and vm.chunk->code is start.
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t byte;
        switch (byte = VM_READ_BYTE()) {
        case OP_CONSTANT: {
            LoxValue constant = VM_READ_CONSTANT();
            push_vm(constant); // "Produce" a value by pushing it onto the stack.
            break;
        }
        case OP_ADD: VM_BINARY_OP(+); break;
        case OP_SUB: VM_BINARY_OP(-); break;
        case OP_MUL: VM_BINARY_OP(*); break;
        case OP_DIV: VM_BINARY_OP(/); break;
        case OP_UNM: 
            push_vm(-pop_vm()); // Could be done w/o pushing and popping explicitly
            break;
        case OP_RET: 
            value_print(pop_vm());
            printf("\n");
            return INTERPRET_OK;
        }
    }
}

LoxInterpretResult interpret_vm(const char *source)
{
    LoxChunk chunk;
    init_chunk(&chunk);
    if (!compile(source, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    LoxInterpretResult result = run_vm();
    free_chunk(&chunk);
    return result;
}

#undef VM_BINARY_OP
#undef VM_READ_BYTE
#undef VM_READ_CONSTANT
