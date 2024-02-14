#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

typedef enum {
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, 
    TOKEN_SEMICOLON, 
    TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SLASH, TOKEN_STAR,

    // 1 or 2 character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,       // '!', '!='
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,     // '=', '=='
    TOKEN_GREATER, TOKEN_GREATER_EQUAL, // '>', '>='
    TOKEN_LESS, TOKEN_LESS_EQUAL,       // '<', '<='

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR, // Indicate to compiler to handle error recovery.
    TOKEN_EOF,
} LoxTokenType;

typedef struct {
    LoxTokenType type;
    const char *start; // Pointer to the token's first character in the source.
    int length; // How many characters does this token take up in the source?
    int line; // Track the line number for error reporting.
} LoxToken;

/* Start the scanner pointing at the first lexeme's first character. */
void scanner_init(const char *source);
LoxToken scanner_scan_token(void);

#endif /* CLOX_SCANNER_H */
