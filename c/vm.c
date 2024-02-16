#include <stdarg.h>
#include "vm.h"
#include "compiler.h"
#include "debug.h"

LoxVM vm = {0}; // For simplicity we'll use global state. Not good practice!

/**
 * Make the stacktop pointer point to the stack base.
 * Note that because our stack is, well, stack-allocated, simple moving the
 * pointer back is more than enough to indicate which memory is used.
 */
static void reset_stack_vm()
{
    vm.stacktop = vm.stack;
}

static void runtime_error_vm(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // Subtract 1 since vm.ip always points to the *next* instruction in line.
    // This is because the interpreter calls `advance()` before executing an
    // instruction, so the failed instruction is the previous one.
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %i] in script\n", line);
    reset_stack_vm();
}

void init_vm(void)
{
    reset_stack_vm();
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

/* Return a value from from the stack but don't pop it. */
static LoxValue peek_vm(int distance)
{
    // Subtract 1 since stacktop points to 1 past the last element.
    return *(vm.stacktop - 1 - distance);
}

/* Return `false` for `nil` and `false`, `true` for everything else. */
static bool isfalsy(LoxValue value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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
 * 
 * III:18.3.2: We pass in a `valuetype` macro in order to interpret the result
 * of the operation.
 */
#define VM_BINARY_OP(valuetype, op)    \
    do { \
        if (!IS_NUMBER(peek_vm(0)) || !IS_NUMBER(peek_vm(1))) { \
            runtime_error_vm("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double rhs = AS_NUMBER(pop_vm()); \
        double lhs = AS_NUMBER(pop_vm()); \
        push_vm(valuetype(lhs op rhs)); \
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
            print_value(*slot);
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
        // Push some literals for Lox datatypes.
        case OP_NIL: push_vm(NIL_VAL); break;
        case OP_TRUE: push_vm(BOOL_VAL(true)); break;
        case OP_FALSE: push_vm(BOOL_VAL(false)); break;
        // Equality operation, not assignment operation!
        case OP_EQUAL: {
            LoxValue rhs = pop_vm();
            LoxValue lhs = pop_vm();
            push_vm(BOOL_VAL(values_equal(lhs, rhs)));
            break;
        }
        // >= and <= will be 2 separate instructions each
        case OP_GREATER: VM_BINARY_OP(BOOL_VAL, >); break;
        case OP_LESS: VM_BINARY_OP(BOOL_VAL, <); break;
        case OP_ADD: VM_BINARY_OP(NUMBER_VAL, +); break;
        case OP_SUB: VM_BINARY_OP(NUMBER_VAL, -); break;
        case OP_MUL: VM_BINARY_OP(NUMBER_VAL, *); break;
        case OP_DIV: VM_BINARY_OP(NUMBER_VAL, /); break;
        case OP_NOT: push_vm(BOOL_VAL(isfalsy(pop_vm()))); break;
        case OP_UNM: 
            // NOTE: This immediately halts the interpreter!
            if (!IS_NUMBER(peek_vm(0))) {
                runtime_error_vm("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push_vm(NUMBER_VAL(-AS_NUMBER(pop_vm())));
            break;
        case OP_RET: 
            print_value(pop_vm());
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
