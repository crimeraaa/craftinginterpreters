package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// Import everything from TokenType.java?
import static com.craftinginterpreters.lox.TokenType.*;

class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int start = 0; // Index of 1st char in lexeme being scanned.
    private int current = 0; // Index of current char in lexeme being scanned.
    private int line = -1; // Use -1 for REPL (interactive prompt).
    private static final Map<String, TokenType> keywords; // Nice and easy!
    
    static {
        keywords = new HashMap<>();
        // Booleans
        keywords.put("true", TK_TRUE);
        keywords.put("false", TK_FALSE);
        keywords.put("and", TK_AND); // Technically also control flow
        keywords.put("or", TK_OR); // Technically also control flow
        // Control flow
        keywords.put("if", TK_IF);
        keywords.put("else", TK_ELSE);
        keywords.put("for", TK_FOR);
        keywords.put("while", TK_WHILE);
        keywords.put("return", TK_RETURN);
        // Types
        keywords.put("fun", TK_FUN);
        keywords.put("class", TK_CLASS);
        keywords.put("nil", TK_NIL);
        keywords.put("var", TK_VAR);
        keywords.put("this", TK_THIS);
        keywords.put("super", TK_SUPER);
        // Builtin
        keywords.put("print", TK_PRINT);
    }

    Scanner(String source) {
        this.source = source;
    }
    
    List<Token> scanTokens() {
        while (!isAtEnd()) {
            // We are at the beginning of the next lexeme.
            this.start = this.current;
            scanToken();
        }
        // end-of-file token to indicate this is where parsing ends.
        this.tokens.add(new Token(TK_EOF, "", null, this.line));
        return this.tokens;
    }
    
    private void scanToken() {
        char c = consumeChar(); // This increments `this.current`.
        switch (c) {
            case '(': addToken(TK_LPAREN); break;
            case ')': addToken(TK_RPAREN); break;
            case '{': addToken(TK_LBRACE); break;
            case '}': addToken(TK_RBRACE); break;
            case ',': addToken(TK_COMMA); break;
            case '.': addToken(TK_PERIOD); break;
            case '-': addToken(TK_MINUS); break;
            case '+': addToken(TK_PLUS); break;
            case ';': addToken(TK_SEMI); break;
            case '*': addToken(TK_STAR); break;
            case '!':
                // both ! and != must be parsed as 1 lexeme, not separate tokens!
                addToken(consumeCharOnlyIf('=') ? TK_COMPARE_NEQ : TK_COMPARE_NOT);
                break;
            case '=':
                // Ditto for = and ==, see above.
                addToken(consumeCharOnlyIf('=') ? TK_COMPARE_EQ : TK_ASSIGN);
                break;
            case '<':
                addToken(consumeCharOnlyIf('=') ? TK_COMPARE_LE : TK_COMPARE_LT);
                break;
            case '>':
                addToken(consumeCharOnlyIf('=') ? TK_COMPARE_GE : TK_COMPARE_GT);
                break;
            case '/':
                // A comment goes until the end of the current line.
                // When newline reached, next call to this function follows.
                if (consumeCharOnlyIf('/')) {
                    while (peekCurrentChar() != '\n' && !isAtEnd()) {
                        consumeChar(); // Comment's content is not relevant to us
                    }
                } else {
                    addToken(TK_SLASH); // Probably division
                }
                break;
            case '?': // cond: TK_*, TK_TERNARY_IF, lhs: TK_*, rhs: TK_*
                if (!peekAheadUntilMatches(':')) {
                    Lox.error(this.line, "Unmatched ternary if.");
                    break;
                }
                addToken(TK_TERNARY_IF);
                scanToken();
                break;
            case ':': 
                // Ensure that something followed the '?'
                Token previous2 = this.tokens.get(this.tokens.size() - 2);
                // Don't confuse TK_IF which is specifically for the "if" keyword
                if (previous2.type == TK_TERNARY_IF) {
                    addToken(TK_TERNARY_ELSE);
                }
                // TODO: Handle incomplete statment like `(1 == 1) ? true :`
                break;
            case ' ':
            case '\r':
            case '\t':
                break; // ignore whitespace so tokens are properly handled.
            case '\n':
                this.line++;
                break;
            case '"':
                consumeString(); // Only works for string literals.
                break;
            default:
                if (isDigit(c)) {
                    consumeNumber(); // Only works for number literals.
                } else if (isAlpha(c)) {
                    consumeIdent(); // May be identifier or keyword though!
                } else {
                    Lox.error(this.line, "Unexpected character.");
                }
                break;
        }
    }
    
    /* Consumes a user's variable identifier OR a language keyword identifier. */
    private void consumeIdent() {
        while (isAlphaNumeric(peekCurrentChar()) && !isAtEnd()) {
            consumeChar();
        }
        addToken(getCurrentTokenType());
    }
    
    private TokenType getCurrentTokenType() {
        String text = this.source.substring(this.start, this.current);
        TokenType type = keywords.get(text);
        // Not a keyword so assume it's a user variable
        if (type == null) {
            return TK_IDENT;
        }
        return type;
    }
    
    /* Consume a number literal: it may be an integer or a decimal number. */
    private void consumeNumber() {
        while (isDigit(peekCurrentChar())) {
            consumeChar();
        }
        // Look for a fractional part.
        if (peekCurrentChar() == '.' && isDigit(peekNextChar())) {
            consumeChar(); // Consume the '.'
            while (isDigit(peekCurrentChar())) {
                consumeChar();
            }
        }
        String text = this.source.substring(this.start, this.current);
        double literal = Double.parseDouble(text);
        addToken(TK_NUMBER, literal);
    }
    
    /* Consumes a string literal. */
    private void consumeString() {
        while (peekCurrentChar() != '"' && !isAtEnd()) {
            if (peekCurrentChar() == '\n') {
                this.line++;
            }
            consumeChar();
        }
        // Syntax error!
        if (isAtEnd()) {
            Lox.error(this.line, "Unterminated string.");
            return;
        }
        // Consume closing '"'.
        consumeChar();

        // Retrieve the string literal but trim its surrounding quotes.
        String literal = this.source.substring(this.start + 1, this.current - 1);
        addToken(TK_STRING, literal);
    }
    
    /* Only consume the character/update counter if it matches. */
    private boolean consumeCharOnlyIf(char expected) {
        if (isAtEnd()) {
            return false;
        }
        if (this.source.charAt(this.current) != expected) {
            return false;
        }
        this.current++;
        return true;
    }
    
    /* One-character lookahead. This is like advance() but without consuming. */
    private char peekCurrentChar() {
        if (isAtEnd()) {
            return '\0';
        } 
        return this.source.charAt(this.current);
    }
    
    /* Two-character lookahead. We could have only one peek function actually. */
    private char peekNextChar() {
        if (this.current + 1 >= this.source.length()) {
            return '\0';
        }
        return this.source.charAt(this.current + 1);
    }
    
    private boolean peekAheadUntilMatches(char expected) {
        return this.source.indexOf(expected, this.current) != -1;
    }
    
    /* Assuming ASCII, based off the following regex: [0-9a-zA-Z_] or \w */
    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }
    
    /* Assuming ASCII, based off the following regex: [a-zA-Z_] or \a */
    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
    }

    /* Assuming ASCII, based off of the following regex: [0-9] or \d */
    private boolean isDigit(char c) {
        return (c >= '0') && (c <= '9'); 
    }
        
    /* Consumes the current character in the source file and updates counters. */
    private char consumeChar() {
        if (isAtEnd()) {
            return '\0';
        }
        return this.source.charAt(this.current++);
    }
    
    /* Grab the text of the current lexeme and create a token for it. */
    private void addToken(TokenType type) {
        addToken(type, null);
    }
    
    /* Overload of above to help handle literals. */
    private void addToken(TokenType type, Object literal) {
        String text = this.source.substring(start, this.current);
        this.tokens.add(new Token(type, text, literal, this.line));
    }
    
    private boolean isAtEnd() {
        return this.current >= this.source.length();
    }
}
