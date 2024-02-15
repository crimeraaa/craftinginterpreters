#include "common.h"
#include "scanner.h"
#include "compiler.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

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

typedef void (*LoxParseFn)(void); // Typedef C function pointer syntax away

/* Represents a single row in our hypothetical parser table. */
typedef struct {
    LoxParseFn prefix; // Function to compile a prefix expression.
    LoxParseFn infix; // Function to compile an infix expression.
    LoxPrecedence precedence;
} LoxParseRule;

LoxParser parser = {0};
LoxChunk *compiling_chunk;

static LoxChunk *current_chunk(void)
{
    return compiling_chunk;
}

static void error_at(LoxToken *token, const char *message)
{
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
static void error(const char *message)
{
    error_at(&parser.previous, message);
}

/* If scanner hands us an error token, inform the user. */
static void error_at_current(const char *message)
{
    error_at(&parser.current, message);
}

/**
 * `parser.previous` is filled with the value of `parser.current`.
 * `parser.current` is populated with whatever the next token is.
 * If we have an error, we look for more errors.
 */
static void advance(void)
{
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
static void consume(LoxTokenType type, const char *message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }
    error_at_current(message);
}

static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

/* Used when we have an opcode followed by a one-byte operand. */
static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_return(void)
{
    emit_byte(OP_RET);
}

/* Add `value` to the current chunk's constants table. */
static uint8_t make_constant(LoxValue value)
{
    int index = add_constant(current_chunk(), value);
    if (index > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)index;
}

/* Generate the code to load the known constant `value`. */
static void emit_constant(LoxValue value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void end_compiler(void)
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    // If had a syntax error, don't dump it as it won't make sense.
    if (!parser.haderror) {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void expression(void);

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

static LoxParseRule *get_rule(LoxTokenType type);

/** 
 * Parse and compile a binary expression of some kind.
 *
 * Assumes the following:
 * `parser.previous`: is a binary operator token.
 * `parser.current`: is the right-hand side of the binary operator. 
 */
static void binary(void)
{
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
    case TOKEN_PLUS:    emit_byte(OP_ADD); break;
    case TOKEN_MINUS:   emit_byte(OP_SUB); break;
    case TOKEN_STAR:    emit_byte(OP_MUL); break;
    case TOKEN_SLASH:   emit_byte(OP_DIV); break;
    default:            return; // Should be unreachable
    }
}

/**
 * Parse and compile a grouping expression.
 * 
 * Assumes the opening '(' was consumed. Does not emit any bytecode on its own,
 * but rather relies on subcalls to other expression parsers to do so.
 */
static void grouping(void)
{
    expression(); // Recursively call to compile expression between the parens.
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

/** 
 * Compile a number literal stored in a LoxToken to an in-memory `double`. 
 * 
 * Assumes `parser.previous` is the token of the number literal in question.
 */
static void number(void)
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(value);
}

/**
 * Parse and compile an unary operation.
 * 
 * Assumes the following:
 * `parser.previous` is a token for an unary operator,
 * `parser.current` is the operand for it.
 */
static void unary(void)
{
    // Keep the operator in this stack frame's memory so that when we recurse,
    // we can apply the correct order of operations as the stack unwinds.
    LoxTokenType optype = parser.previous.type;

    // Recursively call in order to compile the operand. This also leaves the
    // value of the operand/expression result on the top of the stack.
    // So the OP_UNM instruction must be emitted last.
    parse_precedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (optype) {
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
    [TOKEN_BANG]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL,        NULL,       PREC_NONE},
    [TOKEN_EQUAL]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL,        NULL,       PREC_NONE},
    [TOKEN_GREATER]         = {NULL,        NULL,       PREC_NONE},
    [TOKEN_GREATER_EQUAL]   = {NULL,        NULL,       PREC_NONE},
    [TOKEN_LESS]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_LESS_EQUAL]      = {NULL,        NULL,       PREC_NONE},
    // User-defined variable names or value literals
    [TOKEN_IDENTIFIER]      = {NULL,        NULL,       PREC_NONE},
    [TOKEN_STRING]          = {NULL,        NULL,       PREC_NONE},
    [TOKEN_NUMBER]          = {number,      NULL,       PREC_NONE},
    // Lox keywords (alphabetical)
    [TOKEN_AND]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_CLASS]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_ELSE]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_FALSE]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_FOR]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_FUN]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_IF]              = {NULL,        NULL,       PREC_NONE},
    [TOKEN_NIL]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_OR]              = {NULL,        NULL,       PREC_NONE},
    [TOKEN_PRINT]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_RETURN]          = {NULL,        NULL,       PREC_NONE},
    [TOKEN_SUPER]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_THIS]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_TRUE]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_VAR]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_WHILE]           = {NULL,        NULL,       PREC_NONE},
    // Lox compiler interals
    [TOKEN_ERROR]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_EOF]             = {NULL,        NULL,       PREC_NONE},
};

static void parse_precedence(LoxPrecedence precedence)
{
    advance(); // parser.previous, parser.current = parser.current, (next token)

    // Get the prefix parser rule for our parser.current before advance()
    // e.g. emitting constants/literals or parsing left parentheses
    // 
    // Will be one of: `number()`, `string()`, `grouping()`.
    LoxParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expected an expression."); // Might be a syntax error
        return;
    }
    prefix_rule();

    // Look for an infix parser. If yes, the prefix expression was already
    // compiled and that might be an operand for us.
    // This only passes this function's `precedence` is low enough to permit
    // that prefix operator.
    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance(); // parser.previous, parser.curent = parser.current, (next token)

        // Get the infix parser rule for our parser.current before advance()
        // as it could be something like a binary operator (+ - * /)
        LoxParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(); // binary() => operator is at parser.previous
    }
}

static LoxParseRule *get_rule(LoxTokenType type)
{
    return &rules[type];
}

static void expression(void)
{
    parse_precedence(PREC_ASSIGNMENT); // Parse at the lowest precedence level.
}

bool compile(const char *source, LoxChunk *chunk)
{
    init_scanner(source);
    parser.haderror = false;
    parser.panicmode = false;
    compiling_chunk = chunk;

    advance(); // parser.previous = {0}, parser.current = (first token)
    expression(); // Since we now point to first token, let's begin compiling!
    consume(TOKEN_EOF, "Expected end of expression.");
    end_compiler();
    return !parser.haderror;
}
