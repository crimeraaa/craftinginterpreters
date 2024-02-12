package com.craftinginterpreters.lox;

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
