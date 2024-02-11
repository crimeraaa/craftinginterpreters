package com.craftinginterpreters.lox;

/* Part 4.2.1: https://craftinginterpreters.com/scanning.html */
enum TokenType {
    // Single-character tokens. These can never be used for identifiers and such.
    TK_LPAREN, TK_RPAREN, // ( )
    TK_LBRACE, TK_RBRACE, // { } 
    TK_COMMA, TK_PERIOD,           // , .
    TK_MINUS, TK_PLUS,             // - +
    TK_SEMI, TK_SLASH, TK_STAR,    // ; / *

    TK_TERNARY_IF, TK_TERNARY_ELSE, // ? :

    // One or two character tokens. Not all are boolean expressions!
    TK_COMPARE_NOT, TK_COMPARE_NEQ,   // ! !=
    TK_ASSIGN, TK_COMPARE_EQ,         // =, ==
    TK_COMPARE_GT, TK_COMPARE_GE,     // > >=
    TK_COMPARE_LT, TK_COMPARE_LE,     // < <=

    // Literals, also see "lexemes".
    TK_IDENT, TK_STRING, TK_NUMBER,

    // Keywords, all listed here are made up of more than 1 character.
    // In reality, all keywords are reserved words, which, in turn, are just
    // identifiers that the language claims for itself!
    TK_AND, TK_OR, 
    TK_CLASS, TK_FUN,
    TK_IF, TK_ELSE, 
    TK_TRUE, TK_FALSE, TK_NIL, 
    TK_FOR, TK_WHILE,
    TK_VAR, 
    TK_PRINT, TK_RETURN, 
    TK_SUPER, TK_THIS, 

    TK_EOF
}
