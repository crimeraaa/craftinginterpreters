#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "memory.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/** 
 * Store the upper 8 bits of a future 16-bit integer in an 8-bit integer.
 *
 * This is used to split a 16/greater sized integer into 2 8-bit ones so that 
 * they fit in our bytecode instruction set.
 */
#define mask_int16_upper(n)   (((n) >> 8) & 0xFF)

/**
 * Store the lower 8 bits for a future 16-bit integer in an 8-bit integer.
 *
 * This is used to split a 16/greater sized integer into 2 8-bit ones so that 
 * they fit in our bytecode instruction set.
 */
#define mask_int16_lower(n)   ((n) & 0xFF)

typedef struct {
    LoxToken current;
    LoxToken previous;
    bool haderror; // Record whether any errors occured during compilation.
    bool panicmode; // Track whether we're currently in panic mode.
} LoxParser;

/* Enumerated from lowest precedence to highest precedence. */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERMINAL,      // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY,
} LoxPrecedence;

/** 
 * Although most of our functions don't use the bool, we need a uniform function
 * pointer signature for our sanity! It sucks but this is the least worst option.
 */
typedef void (*LoxParseFn)(bool can_assign);

/* Represents a single row in our hypothetical parser table. */
typedef struct {
    LoxParseFn prefix; // Function to compile a prefix expression.
    LoxParseFn infix; // Function to compile an infix expression.
    LoxPrecedence precedence;
} LoxParseRule;

typedef struct {
    LoxToken name; // Resolve identifiers based on the locals' names.
    int depth; // Scope depth of enclosing block at declaration. -1 indicates global.
} LoxLocal;

/**
 * Since our compiler creates variables on the stack in order of their declaration,
 * we just need to know how far down the stack we need to go in order to retrieve
 * those values. These effectively function as our local scope variables.
 */
typedef struct {
    LoxLocal locals[UINT8_COUNT]; // All the locals currently in scope.
    int localcount; // How many local variables are in scope?
    int scopedepth; // How many blocks surround the current bit of code? 0 is the global scope.
} LoxCompiler;

LoxParser parser = {0};
LoxCompiler *current = NULL;
LoxChunk *compiling_chunk;

static LoxChunk *current_chunk(void) {
    return compiling_chunk;
}

static void error_at(LoxToken *token, const char *message) {
    if (parser.panicmode) {
        return; // Suppress other errors to avoid C++ style error vomit
    }
    parser.panicmode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end.");
    } else if (token->type == TOKEN_ERROR) {
        // nothing
    } else {
        fprintf(stderr, " at '%.*s'.", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.haderror = true;
}

/* Report error at the location of the token we just consumed. */
static void error(const char *message) {
    error_at(&parser.previous, message);
}

/* If scanner hands us an error token, inform the user. */
static void error_at_current(const char *message) {
    error_at(&parser.current, message);
}

/**
 * `parser.previous` is filled with the value of `parser.current`.
 * `parser.current` is populated with whatever the next token is.
 * If we have an error, we look for more errors.
 */
static void advance(void) {
    // Need for later when we get a lexeme *after* matching a token.
    parser.previous = parser.current;

    // We use a loop so we can log consecutive errors, if any.
    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }
        error_at_current(parser.current.start);
    }
}

/* Only advance if the current token type is of `type`, otherwise we error out. */
static void consume(LoxTokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    error_at_current(message);
}

/* Check if the parser's current type matches the given one. */
static bool check(LoxTokenType type) {
    return parser.current.type == type;
}

/* Returns true and advances parser only if current type matches the given one. */
static bool match(LoxTokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

static void emit_byte(uint8_t byte) {
    write_chunk(current_chunk(), byte, parser.previous.line);
}

/** 
 * Used when we have an opcode followed by a one-byte operand.
 * Also useful to emit 2 consecutive instructions like OP_EQUAL and OP_NOT.
 */
static void emit_bytes(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

/**
 * Emit the instructions neded to continuously loop.
 * 
 * This emits an unconditional jump, the only way to break out of the loop is
 * handled separately by other code that has a boolean expression and emitted
 * an OP_JUMP_IF_FALSE instruction.
 */
static void emit_loop(int loopstart) {
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loopstart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }
    emit_byte(mask_int16_upper(offset));
    emit_byte(mask_int16_lower(offset));
}

/**
 * III:23.1
 * 
 * Because we don't immediately know how many bytes either if-else branch takes up,
 * we instead fill the jump instruction with a placeholder offset operand.
 * Later, after the condition has been compiled, we go back and replace the
 * placeholder offset with the known amount of bytes using `patch_jump()`.
 * 
 * This is called backpatching, and we use 2 bytes for the jump offset.
 * This allow us to about 65,535 bytes of code which should be mostly enough.
 * 
 * This returns the index into the chunk where the upper 8 bits of the jump offset
 * operand is located.
 */
static int emit_jump(uint8_t instruction) {
    emit_byte(instruction);
    emit_byte(0xFF);
    emit_byte(0xFF);
    return current_chunk()->count - 2;
}

static void emit_return(void) {
    emit_byte(OP_RET);
}

/* Add `value` to the current chunk's constants table. */
static uint8_t make_constant(LoxValue value) {
    int index = add_constant(current_chunk(), value);
    if (index > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)index;
}

/* Generate the code to load the known constant `value`. */
static void emit_constant(LoxValue value) {
    emit_bytes(OP_CONSTANT, make_constant(value));
}

/**
 * III:23.1
 * 
 * This is the second half of generating jump instructions for control flow.
 * We "patch" the placeholder offset generated by `emit_jump()` using the given
 * value.
 * 
 * We do this by emitting 3 bytes beforehand: 1 for the jump instruction and 
 * 2 bytes for its operands. Knowing this, however many bytes the conditional
 * takes up, we can determine the index back into the jump's operands and modify
 * them directly.
 */
static void patch_jump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = current_chunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }
    current_chunk()->code[offset]     = mask_int16_upper(jump);
    current_chunk()->code[offset + 1] = mask_int16_lower(jump);
}

static void init_compiler(LoxCompiler *self) {
    self->localcount = 0;
    self->scopedepth = 0;
    current = self;
}

static void end_compiler(void) {
    emit_return();
#ifdef DEBUG_PRINT_CODE
    // If had a syntax error, don't dump it as it won't make sense.
    if (!parser.haderror) {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

/* Increment the current scope depth counter. */
static void beginscope(void) {
    current->scopedepth++;
}

/** 
 * Decrement the current scope depth counter and pop all its local variables.
 * This is to return the overall stack to the state before the block was entered.
 */
static void endscope(void) {
    current->scopedepth--;
    // Keep going while not global and current local variable is in our scope.
    while (current->localcount > 0 && current->locals[current->localcount - 1].depth > current->scopedepth) {
        emit_byte(OP_POP);
        current->localcount--;
    }
}

static void expression(void);
static void statement(void);
static void declaration(void);
static LoxParseRule *get_rule(LoxTokenType type);

/**
 * Starts at the current token and parses any expression at given precedence
 * level *or* higher.
 *
 * We read the next token then look up the corresponding parse rule.
 * We look up a prefix parser for the current token. The first token is always
 * going to be some kind of prefix expression by definition.
 *
 * It may turn out to be nested as an operand inside 1/more infix expressions.
 */
static void parse_precedence(LoxPrecedence precedence);

/**
 * Interns the user's variable identifier as a constant LoxString.
 * This affects both the VM's chunk constants pool and the its interned strings.
 * 
 * We return the index of the interned string in the constants pool, which will
 * be the argument used by a call to `OP_CONSTANT`.
 */
static uint8_t identifier_constant(LoxToken *name) {
    LoxString *s = copy_string(name->start, name->length);
    return make_constant(make_loxobject(s));
}

/**
 * Since we have LoxTokens and not LoxStrings, we need to do the good 'ol
 * C-style byte-by-byte comparison.
 */
static bool identifiers_equal(LoxToken *lhs, LoxToken *rhs) {
    if (lhs->length != rhs->length) {
        return false;
    }
    return memcmp(lhs->start, rhs->start, lhs->length) == 0;
}

/**
 * Walks the list of tokens currently in scope.
 * If we find a local variable with the given name, great!
 * We go starting from the innermost of the curren scope going outwards.
 * 
 * If we didn't find a variable of this name, we use -1 as a sentinel.
 */
static int resolve_local(LoxCompiler *compiler, LoxToken *name) {
    for (int i = compiler->localcount - 1; i >= 0; i--) {
        LoxLocal *local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

/* Initialize the next available local in the compiler's stack array. */
static void addlocal(LoxToken name) {
    if (current->localcount >= UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    LoxLocal *local = &current->locals[current->localcount++];
    local->name = name;
    local->depth = -1; // Default to -1 until this gets marked initialized.
}

/** 
 * Here, the compiler records the existence of a local variable. 
 * This should only be done for locals since globals are late bound, meaning
 * the compiler doesn't keep track of which declarations of them it's seen.
 * 
 * For locals though, the compiler needs to remember that the variable exists.
 * We do this by appending to the compiler's local variable list in `addlocal()`.
 */
static void declare_variable(void) {
    if (current->scopedepth == 0) {
        return;
    }
    LoxToken *name = &parser.previous;
    // Current scope is always at the end of the array.
    // localcount is affected by `addlocal()` and `endscope()`.
    for (int i = current->localcount - 1; i >= 0; i--) {
        LoxLocal *local = &current->locals[i];
        // Break once we hit a scope outside of the one we were looking at.
        // Because that means we looked at all the variables and found no error.
        if (local->depth != -1 && local->depth < current->scopedepth) {
            break;
        }
        if (identifiers_equal(name, &local->name)) {
            error("A variable with this name already exists in this scope.");
        }
    }
    addlocal(*name);
}

/**
 * If our current scope greater than 0, that means it's a local scope.
 *
 * So we return 0 to indicate to `define_variable()` that it should just mark the
 * variable as initialized and not omit the instructions to load a constant.
 * 
 * This is because local variables aren't looked up by name at runtime.
 */
static uint8_t parse_variable(const char *error_message) {
    consume(TOKEN_IDENTIFIER, error_message);
    declare_variable();
    // Scope depth greater than 0 indicates a local scope of some kind.
    if (current->scopedepth > 0) {
        return 0;
    }
    return identifier_constant(&parser.previous);
}

static void mark_initialized(void) {
    current->locals[current->localcount - 1].depth = current->scopedepth;
}

/**
 * Create the bytecode needed to a load a global variable's identifier.
 * We use the index into the constants table, not the string itself.
 *
 * Locals are temporaries, so we've already initialized them before here.
 * This means we don't need to create a local variable at runtime.
 * The VM stack index, along with some bookkeeping, is more than enough!
 */
static void define_variable(uint8_t global) {
    // scope depth greater than 0 indicates we're at a local scope of some kind.
    if (current->scopedepth > 0) {
        mark_initialized();
        return;
    }
    emit_bytes(OP_DEFINE_GLOBAL, global);
}

/**
 * III:23.2: Logical operators
 * 
 * Parse and compile a short-circuiting logical AND.
 * 
 * Assumes that the left-hand operand has already been compiled and is at the
 * top of the stack.
 * 
 * If truthy, we fall through to the OP_POP instruction and evaluate the 2nd operand.
 * Otherwise, if falsy, we return the first operand as is without evaluating
 * the 2nd operand.
 * 
 * Visualization:
 * 
 *      <left operand expression>
 * +--- OP_JUMP_IF_FALSE
 * |    OP_POP
 * |    <right operand expression>
 * +--> continues;
 */
static void and_(bool can_assign) {
    (void)can_assign;
    int endjump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(endjump);
}

/** 
 * Parse and compile a binary expression of some kind.
 *
 * Assumes the following:
 * `parser.previous`: is a binary operator token.
 * `parser.current`: is the right-hand side of the binary operator. 
 */
static void binary(bool can_assign) {
    (void)can_assign;
    // Keep the operator in this stack frame's memory so that when we recurse,
    // we can apply the correct order of operations as the stack unwinds.
    //
    // One of: `TOKEN_( PLUS | MINUS | STAR | SLASH )`
    LoxTokenType optype = parser.previous.type;
    LoxParseRule *rule = get_rule(optype);

    // Use 1 higher level of precedence since binary operators are left-associative.
    // e.g. 1 + 2 + 3 + 4 should be parsed like ((1 + 2) + 3) + 4.
    //
    // For number literals, e.g. '2' in '1 + 2', this evaluates '2' then returns
    // since precedence of addition is higher than the precedence of a number.
    //
    // This is because parser = {.current = '2', .previous = '+'}.
    parse_precedence((LoxPrecedence)(rule->precedence + 1));

    switch (optype) {
    case TOKEN_EQUAL_EQUAL: emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER:     emit_byte(OP_GREATER); break;
    // (x != y) <=> !(x == y)
    case TOKEN_BANG_EQUAL:  emit_bytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_LESS:        emit_byte(OP_LESS); break;
    // (x >= y) <=> !(x < y)
    case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
    // (x <= y) <=> !(x > y)
    case TOKEN_LESS_EQUAL:  emit_bytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:        emit_byte(OP_ADD); break;
    case TOKEN_MINUS:       emit_byte(OP_SUB); break;
    case TOKEN_STAR:        emit_byte(OP_MUL); break;
    case TOKEN_SLASH:       emit_byte(OP_DIV); break;
    default:                return; // Should be unreachable
    }
}

/** 
 * Emit a boolean or nil literal. 
 *
 * Assumes:
 * 1. `parse_precedence` was called beforehand and consumed the keyword token.
 * 
 * If that's the case we can simply output the proper instruction.
 */
static void literal(bool can_assign) {
    (void)can_assign;
    switch (parser.previous.type) {
    case TOKEN_FALSE:   emit_byte(OP_FALSE); break;
    case TOKEN_NIL:     emit_byte(OP_NIL); break;
    case TOKEN_TRUE:    emit_byte(OP_TRUE); break;
    default:            return; // Should be unreachable.
    }
}

/**
 * Parse and compile a grouping expression.
 * 
 * Assumes the opening '(' was consumed. Does not emit any bytecode on its own,
 * but rather relies on subcalls to other expression parsers to do so.
 */
static void grouping(bool can_assign) {
    (void)can_assign;
    expression(); // Recursively call to compile expression between the parens.
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

/** 
 * Compile a number literal stored in a LoxToken to an in-memory `double`. 
 * 
 * Assumes `parser.previous` is the token of the number literal in question.
 */
static void number(bool can_assign) {
    (void)can_assign;
    double value = strtod(parser.previous.start, NULL);
    emit_constant(make_loxnumber(value)); // Explicitly create a `LoxValue` number.
}

/**
 * III:23.2.1: Logical or operator
 * 
 * A bit trickier than logical AND.
 * 
 * If the left hand side is truthy, we jump over the right hand side without
 * evaluating it. We return the left hand side as is since it's truthy.
 * 
 * Otherwise, we try to evaluate the right hand side. Regardless of its result,
 * we return it as is. So it may be truthy or falsy! It's useful to emulate
 * default values.
 * 
 * Visualization:
 *
 *           <left-operand expression> 
 *      +--  OP_JUMP_IF_FALSE
 *   +--|--  OP_JUMP
 *   |  +--> OP_POP
 *   |       <right-operand expression>
 *   +-----> continues...
 *   
 * This is a bit inefficient compared to our `and_()`, but it's simple enough.
 */
static void or_(bool can_assign) {
    (void)can_assign;
    int elsejump = emit_jump(OP_JUMP_IF_FALSE);
    int endjump = emit_jump(OP_JUMP);

    // Only pop when left hand side is true as we'll do an OP_JUMP_IF_FALSE.
    // This cleans up the stack for the evaluation of the right hand side.
    patch_jump(elsejump);
    emit_byte(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(endjump);
}

/** 
 * Emits a string constant based on the string literal we received.
 * If the string in question is already interned, we just get a pointer to it.
 * Otherwise we allocate a new LoxString and intern it then.
 *
 * This assumes the parser is currently pointing at a well-formed string literal,
 * surrounded only by double quotes.
 * 
 * Note that Lox doesn't support escape sequences.
 */
static void string(bool can_assign) {
    (void)can_assign;
    // We use +1 and -2 to not include the double quotes in the string.
    LoxString *s = copy_string(parser.previous.start + 1, parser.previous.length - 2);
    emit_constant(make_loxobject(s));
}

/**
 * Emit the instructions needed to actually load a variable into the VM.
 * 
 * Right before compiling an expression though, we look for a '=' token.
 * If we have one, we probably need to do assignment to this variable.
 * Otherwise, we can assume it's just retrieving the variable.
 */
static void named_variable(LoxToken name, bool can_assign) {
    uint8_t getop, setop;
    // Prioritize finding local variables with the given name.
    int arg = resolve_local(current, &name);
    // -1 indicates not a local scope, so we need to get/set a global variable.
    if (arg != -1) {
        getop = OP_GET_LOCAL;
        setop = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(&name);
        getop = OP_GET_GLOBAL;
        setop = OP_SET_GLOBAL;
    }
    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(setop, (uint8_t)arg);
    } else {
        emit_bytes(getop, (uint8_t)arg);
    }
}

/**
 * Look up a variable based on the identifier pointed to by `parser.previous`.
 */
static void variable(bool can_assign) {
    named_variable(parser.previous, can_assign);
}

/**
 * Parse and compile an unary operation.
 * 
 * Assumes the following:
 * `parser.previous` is a token for an unary operator,
 * `parser.current` is the operand for it.
 */
static void unary(bool can_assign) {
    (void)can_assign;
    // Keep the operator in this stack frame's memory so that when we recurse,
    // we can apply the correct order of operations as the stack unwinds.
    LoxTokenType optype = parser.previous.type;

    // Recursively call in order to compile the operand. This also leaves the
    // value of the operand/expression result on the top of the stack.
    // So the OP_UNM instruction must be emitted last.
    parse_precedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (optype) {
    case TOKEN_BANG:    emit_byte(OP_NOT); break;
    case TOKEN_MINUS:   emit_byte(OP_UNM); break;
    default:            return; // Unreachable
    }
}

/* Lookup table of function pointers for particular operations and types. */
LoxParseRule rules[TOKEN_COUNT] = {
    // Not really operators in the usual sense
    [TOKEN_LEFT_PAREN]      = {grouping,    NULL,       PREC_NONE},
    [TOKEN_RIGHT_PAREN]     = {NULL,        NULL,       PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL,        NULL,       PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL,        NULL,       PREC_NONE},
    [TOKEN_COMMA]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_DOT]             = {NULL,        NULL,       PREC_NONE},
    // (Mostly) Binary operators
    [TOKEN_MINUS]           = {unary,       binary,     PREC_TERMINAL},
    [TOKEN_PLUS]            = {NULL,        binary,     PREC_TERMINAL},
    [TOKEN_SEMICOLON]       = {NULL,        NULL,       PREC_NONE},
    [TOKEN_SLASH]           = {NULL,        binary,     PREC_FACTOR},
    [TOKEN_STAR]            = {NULL,        binary,     PREC_FACTOR},
    // Boolean operators
    [TOKEN_BANG]            = {unary,       NULL,       PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL,        binary,     PREC_EQUALITY},
    [TOKEN_EQUAL]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL,        binary,     PREC_EQUALITY},
    [TOKEN_GREATER]         = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL]   = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_LESS]            = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]      = {NULL,        binary,     PREC_COMPARISON},
    // User-defined variable names or value literals
    [TOKEN_IDENTIFIER]      = {variable,    NULL,       PREC_NONE},
    [TOKEN_STRING]          = {string,      NULL,       PREC_NONE},
    [TOKEN_NUMBER]          = {number,      NULL,       PREC_NONE},
    // Lox keywords (alphabetical)
    [TOKEN_AND]             = {NULL,        and_,       PREC_AND},
    [TOKEN_CLASS]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_ELSE]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_FALSE]           = {literal,     NULL,       PREC_NONE},
    [TOKEN_FOR]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_FUN]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_IF]              = {NULL,        NULL,       PREC_NONE},
    [TOKEN_NIL]             = {literal,     NULL,       PREC_NONE},
    [TOKEN_OR]              = {NULL,        or_,        PREC_OR},
    [TOKEN_PRINT]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_RETURN]          = {NULL,        NULL,       PREC_NONE},
    [TOKEN_SUPER]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_THIS]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_TRUE]            = {literal,     NULL,       PREC_NONE},
    [TOKEN_VAR]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_WHILE]           = {NULL,        NULL,       PREC_NONE},
    // Lox compiler interals
    [TOKEN_ERROR]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_EOF]             = {NULL,        NULL,       PREC_NONE},
};

static void parse_precedence(LoxPrecedence precedence) {
    advance(); // parser.previous, parser.current = parser.current, (next token)

    // Get the prefix parser rule for our parser.current before advance()
    // e.g. emitting constants/literals or parsing left parentheses
    // 
    // Will be one of: `number()`, `string()`, `grouping()` or `literal()`.
    LoxParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expected an expression."); // Might be a syntax error
        return;
    }
    // If a variable name is nested in an expression with higher precedence,
    // we ignore the '='.
    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    // Look for an infix parser. If yes, the prefix expression was already
    // compiled and that might be an operand for us.
    // This only passes this function's `precedence` is low enough to permit
    // that prefix operator.
    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance(); // parser.previous, parser.curent = parser.current, (next token)

        // Get the infix parser rule for our parser.current before advance()
        // as it could be a binary operator.
        LoxParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign); // binary() => operator is at parser.previous
    }

    // If we didn't consume the '=', that means we couldn't assign a value!    
    if (can_assign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static LoxParseRule *get_rule(LoxTokenType type) {
    return &rules[type];
}

static void expression(void) {
    parse_precedence(PREC_ASSIGNMENT); // Parse at the lowest precedence level.
}

/**
 * Assumes that we already consumed and verified a left bracket token.
 * 
 * A block scope is just a declaration statement boiling down to everything,
 * including other block statements. This includes variable declarations and
 * expression evaluations.
 * 
 * So really, the only difference is in how local variables are looked up.
 * This is why we need state-keeping via the `beginscope()` and `endscope()` 
 * functions.
 */
static void block(void) {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expected ')' after block.");
}

/**
 * Desugaring to allow both `var a;` and `var a = 10`. Default value is nil.
 */
static void var_declaration(void) {
    uint8_t global = parse_variable("Expected a variable name.");
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NIL); // Default assign this variable to nil.
    }
    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    define_variable(global);
}

static void print_statement(void) {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_PRINT);
}

/**
 * III:23.3: While Statements
 * 
 * Like if statements, we have mandatory parentheses as in the C-style.
 * The following jump instruction emulates the breaking on false behaviour.
 * 
 * If true, we pop the value off the stack created by evaluating the expression,
 * then we run a `statement()`. When we hit a false, we break and pop off the
 * stack the value left over from `statement()`.
 * 
 * If false to begin with, we jump to the 2nd pop instruction and just pop off
 * the value created by `expression()`.
 * 
 * Visualization:
 * 
 *          <condition expression> <---+
 *     +--- OP_JUMP_IF_FALSE           |
 *     |    OP_POP                     |
 *     |    <body statement>           |
 *     |    OP_LOOP -------------------+
 *     +--> OP_POP
 *          continues...
 */
static void while_statement(void) {
    // Capture the instruction pointer we need to jump back to the condition.
    int loopstart = current_chunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expected '!' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

    int exitjump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP); // Pop result on top of stack created by `expression()`.
    statement();
    emit_loop(loopstart);

    patch_jump(exitjump);
    emit_byte(OP_POP); // Pop result of either `expression()` or `statement()`.
}

/** 
 * This is just an expression followed by a semicolon. 
 * Semantically, we just evaluate the expression and discard the result.
 * This is because, by themselves, statements do not result in a value.
 */
static void expression_statement(void) {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_POP);
}

/**
 * III:23.4: For Statements
 * 
 * For statements are while while loops with some nice little features, like
 * being their own block scope and declaring local iterator variables.
 * 
 * Visualization:
 * 
 *              <initializer clause>
 *              <condition expression> <--+
 *         +--- OP_JUMP_IF_FALSE          |
 *         |    OP_POP                    |
 *      +--|--- OP_JUMP                   |
 *      |  |    <increment expression> <--|--+
 *      |  |    OP_POP                    |  |
 *      |  |    OP_LOOP ------------------+  | 
 *      |--|--> <body statement>             |
 *         |    OP_LOOP ---------------------+
 *         +--> OP_POP
 *              continues...
 */
static void for_statement(void) {
    beginscope();

    // 1st Clause: Initializer
    // We allow no initializer, local variable declaration, or outer assignment.
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initalizer, so don't do anything.
    } else if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }

    // Keep track of where to jump right whenever the conditional is true.
    int loopstart = current_chunk()->count;

    // 2nd Clause: Conditional
    // We use this in order to exit the loop, similar to a while loop.
    // It can be empty to indicate no condition is needed to break the loop.
    int exitjump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after expression.");

        // Jump out of the loop if the condition is false.
        exitjump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP); // Clean up stack var pushed by `expression()`.
    }
    
    // 3rd Clause: Increment
    // This one is quite messy since we don't have an AST. The problem is that
    // the increment operation must occur AFTER the body. So we have to jump over
    // the increment first, execute the body, then jump back for the iteration.
    if (!match(TOKEN_RIGHT_PAREN)) {
        // This is the main loop that takes us back to the top of the 'for' loop
        // but right before the condition, if there is one.
        int bodyjump = emit_jump(OP_JUMP);
        int incrementstart = current_chunk()->count;
        
        // Compile the iterator expression, commonly an assignment to the iterator
        // It's likely it'll push something to the chunk's stack so pop it off.
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");
        
        // Make loopstart point to the first instruction after the iteration.
        // this makes it so that the loop body is executed before it.
        emit_loop(loopstart);
        loopstart = incrementstart;
        patch_jump(bodyjump);
    }

    // Loop body
    statement();
    emit_loop(loopstart);
    // If no condition (exitjump == -1), we don't have a jump to patch.
    if (exitjump != -1) {
        patch_jump(exitjump);
        emit_byte(OP_POP);
    }
    endscope();
}

/**
 * III:23.1: If statements
 * 
 * By emitting instructions telling the instruction pointer to jump/skip a number
 * of instructions, we exactly emulate how if statements work at a very low level.
 * very low level.
 * 
 * III:23.1.1: Else clauses
 * 
 * If you think about it, else branches require more work, because now we need
 * to accomodate if the condition is true, we run the if-then branch but we then
 * have to jump away from the else-branch. Otherwise, we'll fall through!
 * 
 * In that case we need an unconditional jump. Even if there's no else branch!
 * As a result every if statement actually has an implicit else.
 * 
 * Visualization:
 * 
 *          <condition expression>
 *     +--- OP_JUMP_IF_FALSE
 *     |    OP_POP
 *     |    <then branch statement>
 *  +--|--- OP_JUMP
 *  |  +--> OP_POP
 *  |       <else branch statement>
 *  +-----> continues...
 */
static void if_statement(void) {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");
    
    // How many bytes to offset the instruction pointer by.
    int thenjump = emit_jump(OP_JUMP_IF_FALSE);

    // Zero stack rule: ensure stack is the same size as before.
    // Since we evaluated an expression, it likely pushed a value onto the stack.
    // So we pop if off in order to clean up the stack before the if-then branch.
    //
    // We only reach here when the condition is truthy as it'll fall through.
    emit_byte(OP_POP);
    statement();
    
    // If-then branches MUST have a jump especially when there's an else branch.
    // This is to avoid accidentally falling through when the condition is truthy.
    // 
    // We only reach here when the condition if falsy as it'll always jump here.
    int elsejump = emit_jump(OP_JUMP);
    patch_jump(thenjump);
    emit_byte(OP_POP);

    // If have an explicit else branch, compile it.
    if (match(TOKEN_ELSE)) {
        statement();
    }
    patch_jump(elsejump); // Still need this even if no else branch!
}

/**
 * This boils down to builtin statements (like print) or expressions statements,
 * like variable declarations or whatnot.
 */
static void statement(void) {
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_FOR)) {
        for_statement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginscope();
        block();
        endscope();
    } else {
        expression_statement();
    }
}

/**
 * When an error occured, move to the parser to the next statement boundary.
 * We do this by skipping tokens until we find a preceding token that can end a
 * statement, like a semicolon, or one that begins a statement, like a declaration.
 */
static void synchronize(void) {
    parser.panicmode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN: return;
        default: break; // Do nothing
        }
        advance();
    }
}

/**
 * In Backus-Naur Form grammar, the declaration typically just boils down
 * to parsing some sort of statement, which in turn boils down to some series
 * of expressions and/or more statements.
 */
static void declaration(void) {
    if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        statement();
    }

    if (parser.panicmode) {
        synchronize();
    }
}

bool compile(const char *source, LoxChunk *chunk) {
    init_scanner(source);
    LoxCompiler compiler;
    init_compiler(&compiler);
    parser.haderror = false;
    parser.panicmode = false;
    compiling_chunk = chunk;

    advance(); // parser.previous = {0}, parser.current = (first token)
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    end_compiler();
    return !parser.haderror;
}
