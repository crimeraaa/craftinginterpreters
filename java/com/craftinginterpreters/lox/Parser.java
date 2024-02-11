package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.List;

import static com.craftinginterpreters.lox.TokenType.*;

/**
 * Consumes a flat input sequence and uses Recursive Descent. 
 * 
 * String literals indicate said symbols appear as-is in the code.
 * '*', '+' after groupings surrounded by "()" work as in regex.
 * 
 * program      -> statement* EOF ;
 *
 * statement    -> exprStmt
 *               | printStmt ;
 * exprStmt     -> expression ";"
 * printStmt    -> "print" expression ";" ;
 *
 * expression   -> equality ;
 *
 * equality     -> comparison ( ( "!="|"==" ) comparison )* ;
 * comparison   -> terminal ( ( ">"|">="|"<"|"<=" ) terminal )* ;
 * terminal     -> factor ( ( "-"|"+" ) factor)* ;
 * factor       -> unary ( ( "/"|"*" ) unary )* ;
 * unary        -> ( "!"|"-" ) unary
 *               | primary ;
 * primary      -> NUMBER_LITERAL | STRING_LITERAL 
 *               | "true" 
 *               | "false" 
 *               | "nil"
 *               | "(" expression ")" ;
 */
class Parser {
    private static class ParseError extends RuntimeException {}
    private final List<Token> tokens;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }
    
    /* Attempt to return a usable syntax tree, but that's not guaranteed! */
    List<Stmt> parseProgram() {
        List<Stmt> statements = new ArrayList<>();
        try {
            while (!isAtEnd()) {
                statements.add(parseStatement());
            }
        } catch (ParseError error) {
            // Do nothing for now as the error message was already printed.
        }
        return statements;
    }
    
    private Stmt parseStatement() {
        // It's very important to consume the print token beforehand!!!
        if (consumeTokenIfMatchesAny(TK_PRINT)) {
            return parsePrintStmt();
        }
        return parseExprStmt();
    }
    
    private Stmt parsePrintStmt() {
        Expr value = parseExpression();
        consumeTokenOrThrow(TK_SEMI, "Expected ';' after value.");
        return new Stmt.Print(value);
    }
    
    private Stmt parseExprStmt() {
        Expr expr = parseExpression();
        consumeTokenOrThrow(TK_SEMI, "Expected ';' after expression.");
        return new Stmt.Expression(expr);
    }

    /* Redunant, but may improve readability: expression -> equality */
    private Expr parseExpression() {
        return parseEquality();
    }
    
    /* equality -> comparison ( ( "!="|"==" ) comparison )* ; */
    private Expr parseEquality() {
        // First <comparison> nonterminal.
        Expr expr = parseComparison();

        // Match != or == to indicate a nested expression, otherwise expr ends.
        while (consumeTokenIfMatchesAny(TK_COMPARE_NEQ, TK_COMPARE_EQ)) {
            Token operator = peekPreviousToken();
            Expr right = parseComparison();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }
    
    /* comparison -> terminal ( ( ">"|">="|"<"|"<=" ) terminal )* ; */
    private Expr parseComparison() {
        Expr expr = parseTerminal();
        
        while (consumeTokenIfMatchesAny(TK_COMPARE_GT, TK_COMPARE_GE, TK_COMPARE_LT, TK_COMPARE_LE)) {
            Token operator = peekPreviousToken();
            Expr right = parseTerminal();
            expr = new Expr.Binary(expr, operator, right);
        }
        
        return expr;
    }
    
    /* terminal -> factor ( ( "-"|"+" ) factor )* ; */ 
    private Expr parseTerminal() {
        Expr expr = parseFactor();

        while (consumeTokenIfMatchesAny(TK_MINUS, TK_PLUS)) {
            Token operator = peekPreviousToken();
            Expr right = parseFactor();
            expr = new Expr.Binary(expr, operator, right);
        }
        
        return expr;
    }
    /* factor -> unary ( ("/"|"*") unary )* ; */   
    private Expr parseFactor() {
        Expr expr = parseUnary();

        while (consumeTokenIfMatchesAny(TK_SLASH, TK_STAR)) {
            Token operator = peekPreviousToken();
            Expr right = parseUnary();
            expr = new Expr.Binary(expr, operator, right);
        }
        
        return expr;
    }
    
    /* unary -> ("!"|"-") unary | primary ; */
    private Expr parseUnary() {
        if (consumeTokenIfMatchesAny(TK_COMPARE_NOT, TK_MINUS)) {
            Token operator = peekPreviousToken();
            Expr right = parseUnary();
            return new Expr.Unary(operator, right);
        }
        return primary();
    }

    /**
     * primary -> NUMBER_LITERAL | STRING_LITERAL 
     *          | "true" 
     *          | "false" 
     *          | "nil"
     *          | "(" expression ")" 
     *          | expression ( "," expression )+ ; 
     */
    private Expr primary() {
        if (consumeTokenIfMatchesAny(TK_FALSE)) {
            return parseCommaOrExpr(false);
        }
        if (consumeTokenIfMatchesAny(TK_TRUE)) {
            return parseCommaOrExpr(true);
        }
        if (consumeTokenIfMatchesAny(TK_NIL)) {
            return parseCommaOrExpr(null);
        }
        // Our scanner/lexer already retrieved the in-memory object of this
        if (consumeTokenIfMatchesAny(TK_NUMBER, TK_STRING)) {
            return parseCommaOrExpr(peekPreviousToken().literal);
        }
        if (consumeTokenIfMatchesAny(TK_LPAREN)) {
            Expr expr = parseExpression();
            consumeTokenOrThrow(TK_RPAREN, "Expected ')' after expression.");
            return new Expr.Grouping(expr);
        }

        // We have a token that can't start an expression.
        Token token = peekCurrentToken();
        switch (token.type) {
            // Allow for unary minus though
            case TK_PLUS:
            case TK_SLASH:
            case TK_STAR:
                throw logParseError(token, "Expected a left operand");
            default:
                throw logParseError(token, "Expected an expression.");
        }
    }
    
    /**
     * C-style comma (sep|op)erator. Right associative, lowest precedence.
     * It's analogous to expressions where a single one is expected.
     * All operands are *evaluated* but only the rightmost is returned.
      */
    private Expr parseCommaOrExpr(Object literal) {
        Expr expr = new Expr.Literal(literal);
        if (consumeTokenIfMatchesAny(TK_COMMA)) {
            // Left expression and operand already updated counter propeerly
            return parseExpression();            
        }
        return expr;
    }
    
    
    /* Similar to matchCurrentToken but also throws an error if false. */
    private Token consumeTokenOrThrow(TokenType type, String message) {
        if (matchCurrentToken(type)) {
            return consumeToken();
        }
        throw logParseError(peekCurrentToken(), message);
    }
    
    private ParseError logParseError(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }
    
    /** 
     * Ensure parser is in the right state if an syntax error is caught. 
     * This is because we want to discard tokens until we're right at the
     * beginning of the next statement.
     * 
     * Specifically, we discard tokens until we find a statement boundary.
     * This also helps parse a file with errors by discarding invalid tokens.
     */
    private void synchronizeParser() {
        consumeToken();

        while (!isAtEnd()) {
            // Semicolons indicate a finished statement. (or they SHOULD at least)
            if (peekPreviousToken().type == TK_SEMI) {
                return;
            }
            // Any of the labelled tokens indicate we're about to start a statement.
            // For the rest, we just keep consuming tokens.
            switch (peekCurrentToken().type) {
                case TK_CLASS:
                case TK_FUN:
                case TK_VAR:
                case TK_IF:
                case TK_FOR:
                case TK_WHILE:
                case TK_PRINT:
                case TK_RETURN: return;
                default: break;
            }
            consumeToken();
        }
    }
    
    /**
     * If our current token matches any in `types`, consume it and return true.
     * Otherwise, leave it alone and return false.
     * 
     * Returns true only if it matched one of the types AND we consumed the token.
     */
    private boolean consumeTokenIfMatchesAny(TokenType ...types) {
        for (TokenType type : types) {
            if (matchCurrentToken(type)) {
                consumeToken();
                return true;
            }
        } 
        return false;
    }
   
    /* Look if the current token is of `type` but do not consume it. */
    private boolean matchCurrentToken(TokenType type) {
        if (isAtEnd()) {
            return false;
        }
        return peekCurrentToken().type == type;
    }
    
    /* Consume the current token then return it. */
    private Token consumeToken() {
        if (!isAtEnd()) {
            this.current++;
        }
        // Need this since `this.current` was incremented.
        return peekPreviousToken();
    }
    
    private boolean isAtEnd() {
        return peekCurrentToken().type == TK_EOF;
    }

    private Token peekCurrentToken() {
        return peekOffsetToken(0);
    }
    
    private Token peekPreviousToken() {
        return peekOffsetToken(-1);
    }
    
    private Token peekOffsetToken(int offset) {
        return tokens.get(current + offset);
    }
}
