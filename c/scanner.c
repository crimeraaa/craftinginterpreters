#include <ctype.h>      /* isdigit, isalpha, isalnum */
#include "common.h"
#include "scanner.h"

typedef struct {
    const char *start; // Pointer to beginning of current lexeme being scanned.
    const char *current; // Pointer to current character being looked at.
    int line; // Track line number for error reporting.
} LoxScanner;

LoxScanner scanner = {0};

void init_scanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

/* Source string MUST be a nul terminated C-string. */
static bool is_at_end(void)
{
    return *scanner.current == '\0';
}

/** 
 * Increments the scanner's `.current` character pointer and returns the value
 * of the character at the point before the increment.
 */
static char advance()
{
    scanner.current++;
    return *(scanner.current - 1); // Since incremented, previous current was -1
}

/* Return the current character without consuming it. */
static char peek_current(void)
{
    return *scanner.current;
}

static char peek_next(void)
{
    if (is_at_end()) {
        return '\0';
    }
    return *(scanner.current + 1);
}

/* If scanner.current == expected, also increment scanner.current pointer. */
static bool match(char expected)
{
    if (is_at_end()) {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

/* Create a LoxToken based off the scanner's current state and given type. */
static LoxToken make_token(LoxTokenType type)
{
    LoxToken token;
    token.type = type;
    token.start = scanner.start;
    token.length = scanner.current - scanner.start; // Capture lexeme length
    token.line = scanner.line;
    return token;
}

/* Please ensure that this is called with only readonly string literals! */
static LoxToken error_token(const char *message)
{
    LoxToken token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner.line;
    return token;
}

static void skip_whitespace(void)
{
    for (;;) {
        char c = peek_current();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break; // Forgot to include this, was breaking stuff!!
        case '/': // Comment aren't whitespace but may as well cover them here.
            if (peek_next() == '/') {
                while (peek_current() != '\n' && !is_at_end()) {
                    advance();
                }
            } else {
                return; // Was a single slash so leave it unconsumed here.
            }
            break;
        default:
            return; // Once we hit a nonwhitespace, get outta here.
        }
    }
}

/**
 * Implement a simple Trie data structure to induce a Deterministic Finite State
 * Automaton (DFA). In other words, this is the innards of a state machine!
 *
 * Try to see if given substring in scanner's source code represents a keyword.
 * If it doesn't we assume that this is a user-defined keyword.
 *
 * @param start     Index into the main keyword where comparison begins.
 *                  e.g., start == 1 if we matched 'a' for "and".
 *                  or start == 2 if we matched 'f' and 'o' in "for".
 * @param length    Length of `rest`. Used to calculate the keyword's length.
 * @param rest      A string literal containing the remaining uncompared characters
 *                  for the main keyword, e.g. "eturn" from "return".
 * @param type      LoxTokenType for the keyword we want to match.
 */
static LoxTokenType check_keyword(int start, int length, const char *rest, LoxTokenType type)
{
    // Only do the memcmp if substring length matches keyword's string length.
    if (scanner.current - scanner.start == start + length) {
        if (memcmp(scanner.start + start, rest, length) == 0) {
            return type;
        }
    }
    return TOKEN_IDENTIFIER;
}

/**
 * Could be a user variable name or a keyword. We used a somewhat hardcoded
 * Trie data structure and some slightly clever searching to look up things
 * really really fast.
 */
static LoxTokenType identifier_type(void)
{
    // Check if initial letter of the current lexeme may be a keyword's start.
    // If it is, check the rest of the lexeme's characters versus a keyword.
    switch (*scanner.start) {
    case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
    case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        // None of our keywords have a length of 1 exactly.
        if (scanner.current - scanner.start > 1) {
            switch (*(scanner.start + 1)) {
            case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
            case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
            }
        }
    case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
    case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        // None of our keywords have a length of 1 exactly.
        if (scanner.current - scanner.start > 1) {
            switch(*(scanner.start + 1)) {
            case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
            case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
            }
        }
    case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static LoxToken scan_identifier(void)
{
    // Allow [a-zA-Z0-9_] for identifiers past the first character.
    while (isalnum(peek_current()) || (peek_current() == '_')) {
        advance();
    }
    return make_token(identifier_type());
}

static LoxToken scan_string(void)
{
    while (peek_current() != '"' && !is_at_end()) {
        // Allow multi-line strings.
        if (peek_current() == '\n') {
            scanner.line++;
        }
        advance();
    }
    if (is_at_end()) {
        return error_token("Unterminated string.");
    }
    // Consume the closing double quote.
    advance();
    return make_token(TOKEN_STRING);
}

static LoxToken scan_number(void)
{
    while (isdigit(peek_current())) {
        advance();
    }

    // Look for a fractional part.
    if (peek_current() == '.' && isdigit(peek_next())) {
        // Consume the '.' so we can start the fractional section.
        advance();
        while (isdigit(peek_current())) {
            advance();
        }
    }
    return make_token(TOKEN_NUMBER);
}

LoxToken scan_token(void)
{
    skip_whitespace();

    scanner.start = scanner.current; // Assume we're starting a new token.
    if (is_at_end()) {
        return make_token(TOKEN_EOF); // Signal compiler to stop looking for more.
    }

    char c = advance();
    // C Standard Library isalpha only covers the regex: [a-zA-Z].
    // See: https://en.cppreference.com/w/c/string/byte/isalpha
    if (isalpha(c) || c == '_') {
        return scan_identifier();
    }
    if (isdigit(c)) {
        return scan_number();
    }

    switch (c) {
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}': return make_token(TOKEN_RIGHT_BRACE);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ',': return make_token(TOKEN_COMMA);
    case '.': return make_token(TOKEN_DOT);
    case '-': return make_token(TOKEN_MINUS);
    case '+': return make_token(TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
    case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return scan_string();
    }
    return error_token("Unexpected character.");
}

#undef isident
