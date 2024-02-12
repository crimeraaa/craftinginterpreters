package com.craftinginterpreters.lox;

/* Part 4.2.1: https://craftinginterpreters.com/scanning.html */
enum TokenType {
    // Single-character tokens. These can never be used for identifiers and such.
    LEFT_PAREN,     RIGHT_PAREN,  // ( )
    LEFT_BRACE,     RIGHT_BRACE,  // { } 
    OPERATOR_COMMA, OPERATOR_DOT, // , .
    OPERATOR_SUB,   OPERATOR_ADD, // - +
    OPERATOR_DIV,   OPERATOR_MUL, // / *
    OPERATOR_SEMI,                // ;
 
    // One or two character tokens. Not all are boolean expressions!
    OPERATOR_NOT,   OPERATOR_NEQ, // ! !=
    OPERATOR_ASSIGN,OPERATOR_EQ,  // = ==
    OPERATOR_GT,    OPERATOR_GE,  // > >=
    OPERATOR_LT,    OPERATOR_LE,  // < <=

    // Literals, also see "lexemes".
    IDENTIFIER, 
    STRING_LITERAL, 
    NUMBER_LITERAL,

    // Keywords, all listed here are made up of more than 1 character.
    // In reality, all keywords are reserved words, which, in turn, are just
    // identifiers that the language claims for itself!
    KEYWORD_VAR, 
    KEYWORD_CLASS,  KEYWORD_FUN,
    KEYWORD_AND,    KEYWORD_OR, 
    KEYWORD_IF,     KEYWORD_ELSE, 
    KEYWORD_TRUE,   KEYWORD_FALSE, 
    KEYWORD_NIL, 
    KEYWORD_FOR,    KEYWORD_WHILE,
    // KEYWORD_PRINT,  
    KEYWORD_RETURN, 
    KEYWORD_SUPER,  KEYWORD_THIS, 

    END_OF_FILE
}

class Token {
    final TokenType type;
    final String lexeme; // Variable identifier or source code token.
    final Object literal; // Actual value in memory string and number literals.
    final int line;

    /* Seems like the constructor? */
    Token(TokenType type, String lexeme, Object literal, int line) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
    }

    public String toString() {
        return type + " " + lexeme + " " + literal;
    }
}
