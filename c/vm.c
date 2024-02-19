#include <stdarg.h>
#include "vm.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "value.h"

LoxVM vm = {0}; // For simplicity we'll use global state. Not good practice!

/**
 * Make the stacktop pointer point to the stack base.
 * Note that because our stack is, well, stack-allocated, simply moving the
 * pointer back is more than enough to indicate which memory is used.
 */
static void reset_stack_vm() {
    vm.stacktop = vm.stack;
}

static void runtime_error_vm(const char *format, ...) {
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

void init_vm(void) {
    reset_stack_vm();
    vm.objects = NULL;
    init_table(&vm.globals); // III:21.2: Variable Declarations
    init_table(&vm.strings);
}

void free_vm(void) {
    free_table(&vm.globals); // III:21.2: Variable Declarations
    free_table(&vm.strings);
    free_objects(); 
}

void push_vm(LoxValue value) {
    *vm.stacktop = value;
    vm.stacktop++;
}

LoxValue pop_vm(void) {
    vm.stacktop--;
    return *vm.stacktop;
} 

/* Return a value from from the stack but don't pop it. */
static LoxValue peek_vm(int distance) {
    // Subtract 1 since stacktop points to 1 past the last element.
    return *(vm.stacktop - 1 - distance);
}

/** 
 * Return true if value is an instance of nil, or the boolean `false`.
 * Otherwise, we return false since it's not a falsy type.
 */
static bool isfalsy(LoxValue value) {
    return is_loxnil(value) || (is_loxbool(value) && !as_loxbool(value));
}

static void concatenate(void) {
    // Remember that rhs was most recently pushed to the pop of the stack.
    LoxString *rhs = as_loxstring(pop_vm());
    LoxString *lhs = as_loxstring(pop_vm());

    int length = lhs->length + rhs->length;
    char *buffer = allocate(char, length + 1); 

    memcpy(buffer, lhs->buffer, lhs->length);
    // Start writing at the end of the first string in our buffer,
    // hence we use pointer arithmetic to start at the correct offset.
    memcpy(buffer + lhs->length, rhs->buffer, rhs->length);
    buffer[length] = '\0';

    LoxString *result = take_string(buffer, length);
    push_vm(make_loxobject(result));
}

/** 
 * Return the current value of `vm.ip` but increment the pointer afterwards. 
 */
#define read_byte()     (*vm.ip++)


/**
 * Reads the constant value located at the constants pool, using the current
 * instruction pointer as the index. The instruction pointer is incremented.
 */
#define read_constant() (vm.chunk->constants.values[read_byte()])

/**
 * This macro is very terse! Let's break it down. Overall we're just taking two
 * 8-bit values from the chunk and building a 16-bit value from them.
 * 
 * 1. vm.ip += 2:   This does exactly what it says, except that since this is a
 *                  comma-separated expression, it won't be returned. Remember
 *                  that in C, comma-separated expressions returns only the one
 *                  expression at the rightmost comma.
 *
 * 2. vm.ip[-2]<<8  We know that the upper 8 bits of the jump operand are here.
 *                  By bitshifting 8 bits to the left, we move all the bits of 
 *                  this 8-bit value to the upper 8 bits of a 16 bit one.
 *
 * 3. | vm.ip[-1]   We bitwise OR the result of #2. with the lower 8 bits of the
 *                  jump operand. Since we bitshifted left previously, all the
 *                  lower bits of the previous operation are now zeroed out.
 *                  When we do a bitwise OR, both operands' nonzero bits are 
 *                  copied over into the result. E.g. `0b1001 | 0b1011 = 0b1011`.
 */
#define read_short()    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))

/**
 * Assuming that the top of the stack contains a Lox string, immediately read
 * it as such. This may not end well!
 */
#define read_string()   (as_loxstring(read_constant()))

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
#define make_binary_op(valuetype, op) \
    do { \
        if (!is_loxnumber(peek_vm(0)) || !is_loxnumber(peek_vm(1))) { \
            runtime_error_vm("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double rhs = as_loxnumber(pop_vm()); \
        double lhs = as_loxnumber(pop_vm()); \
        push_vm(valuetype(lhs op rhs)); \
    } while(false)

static LoxInterpretResult run_vm(void) {
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
        switch (byte = read_byte()) {
        case OP_CONSTANT: push_vm(read_constant()); break;
        case OP_DEFINE_GLOBAL: {
            LoxString *name = read_string();
            table_set(&vm.globals, name, peek_vm(0));
            pop_vm();
            break;
        }
        case OP_GET_GLOBAL: {
            LoxString *name = read_string();
            LoxValue value; // Out parameter of `table.get`.
            if (!table_get(&vm.globals, name, &value)) {
                runtime_error_vm("Undefined variable '%s'.", name->buffer);
                return INTERPRET_RUNTIME_ERROR;
            }
            push_vm(value);
            break;
        }
        case OP_SET_GLOBAL: {
            LoxString *name = read_string();
            // Bit wasteful, but gets the job done.
            // table_set() returns true if we created a new key, meaning
            // this global variable wasn't declared beforehand!
            if (table_set(&vm.globals, name, peek_vm(0))) {
                table_delete(&vm.globals, name);
                runtime_error_vm("Undefined variable '%s'.", name->buffer);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        // Since locals are pushed to the stack in order of their declaration,
        // we can read them in a similar way using the instruction pointer.
        // Pushing the same local value again seems redundant, but we need to do
        // this since other bytecode instructions only poke at the stacktop.
        case OP_GET_LOCAL: {
            uint8_t slot = read_byte();
            push_vm(vm.stack[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = read_byte();
            vm.stack[slot] = peek_vm(0);
            break;
        }
        // Used by expression statements to discard their results.
        case OP_POP: pop_vm(); break;
        // Push some literals for Lox datatypes.
        case OP_NIL:   push_vm(make_loxnil); break;
        case OP_TRUE:  push_vm(make_loxbool(true)); break;
        case OP_FALSE: push_vm(make_loxbool(false)); break;
        // Equality operation, not assignment operation!
        case OP_EQUAL: {
            LoxValue rhs = pop_vm();
            LoxValue lhs = pop_vm();
            push_vm(make_loxbool(values_equal(lhs, rhs)));
            break;
        }
        // >= and <= will be 2 separate instructions each
        case OP_GREATER: make_binary_op(make_loxbool, >); break;
        case OP_LESS:    make_binary_op(make_loxbool, <); break;
        case OP_ADD: {
            if (is_loxstring(peek_vm(0)) && is_loxstring(peek_vm(1))) {
                concatenate();
            } else if (is_loxnumber(peek_vm(0)) && is_loxnumber(peek_vm(1))) {
                // Remember that rhs was most recently pushed to the stack.
                double rhs = as_loxnumber(pop_vm());
                double lhs = as_loxnumber(pop_vm());
                push_vm(make_loxnumber(lhs + rhs));
            } else {
                runtime_error_vm("Operands must be 2 numbers or 2 strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUB: make_binary_op(make_loxnumber, -); break;
        case OP_MUL: make_binary_op(make_loxnumber, *); break;
        case OP_DIV: make_binary_op(make_loxnumber, /); break;
        case OP_NOT: push_vm(make_loxbool(isfalsy(pop_vm()))); break;
        case OP_UNM: 
            // NOTE: This immediately halts the interpreter!
            if (!is_loxnumber(peek_vm(0))) {
                runtime_error_vm("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push_vm(make_loxnumber(-as_loxnumber(pop_vm())));
            break;
        case OP_PRINT: {
            // When interpreter reaches this instruction, the singular argument
            // to print should be at the top of the stack.
            print_value(pop_vm());
            printf("\n");
            break;
        }
        // We don't need to check the condition, we always apply the offset.
        case OP_JUMP: {
            uint16_t offset = read_short();
            vm.ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            // Use some bit twiddling fun to turn 2 uint8's into 1 uint16.
            uint16_t offset = read_short();
            // Evaluate the condition at runtime. If false, we do jump!
            if (isfalsy(peek_vm(0))) {
                vm.ip += offset;
            }
            break;
        }
        // We decrement vm.ip since we want to jump backwards.
        // As is, the loop jumps unconditionally. Breaking out is another case.
        case OP_LOOP: {
            uint16_t offset = read_short();
            vm.ip -= offset;
            break;
        }
        case OP_RET: 
            // Exit interpreter.
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

#undef make_binary_op
#undef read_byte
#undef read_constant
#undef read_string
#undef read_short
