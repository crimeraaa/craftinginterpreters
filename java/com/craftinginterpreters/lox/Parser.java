package com.craftinginterpreters.lox;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// Specifically has to be import static to bring enum members into global scope.
import static com.craftinginterpreters.lox.TokenType.*;

/**
 * Consumes a flat input sequence and uses Recursive Descent. 
 * 
 * String literals indicate said symbols appear as-is in the code.
 * '*', '+' after groupings surrounded by "()" work as in regex.
 * 
 * program      -> declaration* EOF ;
 * 
 * declaration  -> funDecl
 *               | varDecl
 *               | statement ;
 *
 * funDecl      -> "fun" function ;
 * function     -> IDENTIFIER "(" parameters? ")" block;
 * parameters   -> IDENTIFIER ( "," IDENTIFIER )* ;
 *
 * varDecl      -> "var" IDENTIFIER ( "=" expression )? ";" ;
 *
 * statement    -> exprStmt
 *               | forStmt
 *               | ifStmt 
 *               | printStmt 
 *               | whileStmt
 *               | block ;
 * forStmt      -> "for" "(" ( varDecl | exprStmt | ";" ) ")"
 *                  expression? ";"
 *                  expression? ")" statement ;
 * whileStmt    -> "while" "(" expression ")" statement ;
 * ifStmt       -> "if" "(" expression ")" statement ( "else" statement )? ;
 * 
 * block        -> "{" declaration* "}" ;
 * 
 * exprStmt     -> expression ";"
 * printStmt    -> "print" expression ";" ;
 *
 * expression   -> assignment ;
 * assignment   -> IDENTIFIER "=" assignment
 *               | logic_or ;
 * 
 * logic_or     -> logic_and ( "or" logic_and )* ;
 * logic_and    -> equality ( "and" equality )* ;
 *
 * equality     -> comparison ( ( "!="|"==" ) comparison )* ;
 * comparison   -> terminal ( ( ">"|">="|"<"|"<=" ) terminal )* ;
 * terminal     -> factor ( ( "-"|"+" ) factor)* ;
 * factor       -> unary ( ( "/"|"*" ) unary )* ;
 *
 * unary        -> ( "!"|"-" ) unary
 *               | call ;
 * call         -> primary ( "(" arguments? ")" )* ;
 * arguments    -> expression ( "," expression )* ;
 * primary      -> NUMBER_LITERAL | STRING_LITERAL 
 *               | "true" | "false" | "nil"
 *               | "(" expression ")" 
 *               | IDENTIFIER ;
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
                statements.add(parseDeclaration());
            }
        } catch (ParseError error) {
            // Do nothing for now as the error message was already printed.
        }
        return statements;
    }
    
    private Stmt parseDeclaration() {
        try {
            if (consumeTokenIfMatchesAny(KEYWORD_FUN)) {
                return parseFunction("function");
            }
            if (consumeTokenIfMatchesAny(KEYWORD_VAR)) {
                return parseVarDecl();
            }
            return parseStatement();
        } catch (ParseError error) {
            synchronizeParser(); // Need when parser panics
            return null;
        }
    }
    
    private Stmt parseStatement() {
        if (consumeTokenIfMatchesAny(KEYWORD_FOR)) {
            return parseForStmt();
        }
        // It's very important to consume the print token beforehand!!!
        if (consumeTokenIfMatchesAny(KEYWORD_IF)) {
            return parseIfElseStmt();
        }
        // if (consumeTokenIfMatchesAny(KEYWORD_PRINT)) {
        //     return parsePrintStmt();
        // }
        if (consumeTokenIfMatchesAny(KEYWORD_WHILE)) {
            return parseWhileStmt();
        }
        if (consumeTokenIfMatchesAny(LEFT_BRACE)) {
            return new Stmt.Block(parseBlock());
        }
        return parseExprStmt();
    }
    
    private Stmt parseForStmt() {
        consumeTokenOrThrow(LEFT_PAREN, "Expected '(' after 'for'.");

        // Left-hand expression in the for loop, executed exactly once at start.
        Stmt initializer;
        if (consumeTokenIfMatchesAny(OPERATOR_SEMI)) {
            initializer = null;
        } else if (consumeTokenIfMatchesAny(KEYWORD_VAR)) {
            initializer = parseVarDecl();
        } else {
            initializer = parseExprStmt();
        }
        
        Expr condition = null;
        if (!matchCurrentToken(OPERATOR_SEMI)) {
            condition = parseExpression();
        }
        consumeTokenOrThrow(OPERATOR_SEMI, "Expected ';' after loop condition.");
        
        Expr increment = null;
        if (!matchCurrentToken(RIGHT_PAREN)) {
            increment = parseExpression();
        }
        consumeTokenOrThrow(RIGHT_PAREN, "Expected ')' after for clause.");
        
        // Allow for nested for's, if-else's, prints, scopes, etc.
        Stmt body = parseStatement();
        
        // If we have something to increment with, use it.
        if (increment != null) {
            body = new Stmt.Block(Arrays.asList(body, new Stmt.Expression(increment)));
        }
        
        // If no condition given, assume the user wants an infinite loop.
        if (condition == null) {
            condition = new Expr.Literal(true);
        }
        
        // Given the above, we can now construct an in-memory while loop.
        body = new Stmt.While(condition, body);
        
        // If there's an initializer, we run it before the loop is executed.
        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }
        return body;
    }
    
    /* For nested and unscoped if-else, each else binds to the previous if. */
    private Stmt parseIfElseStmt() {
        consumeTokenOrThrow(LEFT_PAREN, "Expected '(' after 'if'.");
        Expr condition = parseExpression();
        consumeTokenOrThrow(RIGHT_PAREN, "Expected ')' after if condition.");

        Stmt thenBranch = parseStatement();
        Stmt elseBranch = null;
        if (consumeTokenIfMatchesAny(KEYWORD_ELSE)) {
            elseBranch = parseStatement();
        }
        return new Stmt.If(condition, thenBranch, elseBranch);
    }
    
    // private Stmt parsePrintStmt() {
    //     Expr value = parseExpression();
    //     consumeTokenOrThrow(OPERATOR_SEMI, "Expected ';' after value.");
    //     return new Stmt.Print(value);
    // }
    
    private Stmt parseWhileStmt() {
        consumeTokenOrThrow(LEFT_PAREN, "Expected '(' after 'while'.");
        Expr condition = parseExpression();
        consumeTokenOrThrow(RIGHT_PAREN, "Expected ')' after condition.");
        Stmt body = parseStatement();
        return new Stmt.While(condition, body);
    }
    
    private Stmt parseVarDecl() {
        Token name = consumeTokenOrThrow(IDENTIFIER, "Expected a variable name.");
        Expr initializer = null; // If no '=' found, default to null.
        // If '=' found, consume it then parse whatever the assignment is.
        if (consumeTokenIfMatchesAny(OPERATOR_ASSIGN)) {
            initializer = parseExpression();
        }
        consumeTokenOrThrow(OPERATOR_SEMI, "Expected ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }
    
    private Stmt parseExprStmt() {
        Expr expr = parseExpression();
        consumeTokenOrThrow(OPERATOR_SEMI, "Expected ';' after expression.");
        return new Stmt.Expression(expr);
    }
    
    /**
     * kind may be one of "function" or "method" to differentiate freestanding 
     * functions and class methods.
     */
    private Stmt.Function parseFunction(String kind) {
        Token name = consumeTokenOrThrow(IDENTIFIER, "Expected " + kind + "name.");
        consumeTokenOrThrow(LEFT_PAREN, "Expected '( after " + kind + " name.");
        List<Token> parameters = new ArrayList<>();
        if (!matchCurrentToken(RIGHT_PAREN)) {
            do {
                if (parameters.size() >= 255) {
                    logParseError(peekCurrentToken(), "Can't have more than 255 parameters.");
                }
                parameters.add(consumeTokenOrThrow(IDENTIFIER, "Expected parameter name."));
            } while (consumeTokenIfMatchesAny(OPERATOR_COMMA));
        }
        consumeTokenOrThrow(RIGHT_PAREN, "Expected ')' after parameters.");
        consumeTokenOrThrow(LEFT_BRACE, "Expected '{' before " + kind + " body.");
        // parseBlock assumes the left brace was already consumed.
        List<Stmt> body = parseBlock();
        return new Stmt.Function(name, parameters, body);
    }
    
    private List<Stmt> parseBlock() {
        List<Stmt> statements = new ArrayList<>();
        while (!matchCurrentToken(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(parseDeclaration());
        }
        consumeTokenOrThrow(RIGHT_BRACE, "Expected '}' after block.");
        return statements;
    }

    /* Redunant, but may improve readability: expression -> equality */
    private Expr parseExpression() {
        return parseAssignment();
    }
    
    /** 
     * lvalues like the 2nd expression in `var a = 13; a = 42;` are special cases. 
     * So Expr.Assign has a special case where it also keeps the left-hand side
     * in order to evaluate assignment of alread existing variables.
     */
    private Expr parseAssignment() {
        // Parses and evaluate left-side in full, may be an expression!
        Expr expr = parseOrExpr(); 
        // If '=' found this means we need to assign something to left side.
        if (consumeTokenIfMatchesAny(OPERATOR_ASSIGN)) {
            Token equals = peekPreviousToken();
            Expr value = parseAssignment(); // Yay recursion
            // If left-hand side is a variable name, convert rvalue to lvalue
            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, value);
            }
            
            logParseError(equals, "Invalid assignment target.");
        }
        return expr;
    }
    
    /* In Lox, or is a higher precedence than and. For some reason... */
    private Expr parseOrExpr() {
        Expr expr = parseAndExpr();

        while (consumeTokenIfMatchesAny(KEYWORD_OR)) {
            Token operator = peekPreviousToken();
            Expr right = parseAndExpr();
            expr = new Expr.Logical(expr, operator, right);
        }
        
        return expr;
    }
    
    private Expr parseAndExpr() {
        Expr expr = parseEquality();

        while (consumeTokenIfMatchesAny(KEYWORD_AND)) {
            Token operator = peekPreviousToken();
            Expr right = parseEquality();
            expr = new Expr.Logical(expr, operator, right);
        }
        
        return expr;
    }
    
    /* equality -> comparison ( ( "!="|"==" ) comparison )* ; */
    private Expr parseEquality() {
        // First <comparison> nonterminal.
        Expr expr = parseComparison();

        // Match != or == to indicate a nested expression, otherwise expr ends.
        while (consumeTokenIfMatchesAny(OPERATOR_NEQ, OPERATOR_EQ)) {
            Token operator = peekPreviousToken();
            Expr right = parseComparison();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }
    
    /* comparison -> terminal ( ( ">"|">="|"<"|"<=" ) terminal )* ; */
    private Expr parseComparison() {
        Expr expr = parseTerminal();
        
        while (consumeTokenIfMatchesAny(OPERATOR_GT, OPERATOR_GE, OPERATOR_LT, OPERATOR_LE)) {
            Token operator = peekPreviousToken();
            Expr right = parseTerminal();
            expr = new Expr.Binary(expr, operator, right);
        }
        
        return expr;
    }
    
    /* terminal -> factor ( ( "-"|"+" ) factor )* ; */ 
    private Expr parseTerminal() {
        Expr expr = parseFactor();

        while (consumeTokenIfMatchesAny(OPERATOR_SUB, OPERATOR_ADD)) {
            Token operator = peekPreviousToken();
            Expr right = parseFactor();
            expr = new Expr.Binary(expr, operator, right);
        }
        
        return expr;
    }
    /* factor -> unary ( ("/"|"*") unary )* ; */   
    private Expr parseFactor() {
        Expr expr = parseUnary();

        while (consumeTokenIfMatchesAny(OPERATOR_DIV, OPERATOR_MUL)) {
            Token operator = peekPreviousToken();
            Expr right = parseUnary();
            expr = new Expr.Binary(expr, operator, right);
        }
        
        return expr;
    }
    
    /* unary -> ("!"|"-") unary | primary ; */
    private Expr parseUnary() {
        if (consumeTokenIfMatchesAny(OPERATOR_NOT, OPERATOR_SUB)) {
            Token operator = peekPreviousToken();
            Expr right = parseUnary();
            return new Expr.Unary(operator, right);
        }
        return parseCall();
    }
    
    private Expr parseCall() {
        Expr expr = parsePrimary();

        while (true) {
            // Parse a calling expr using the previous parsed expr as callee.
            if (consumeTokenIfMatchesAny(LEFT_PAREN)) {
                expr = finishCall(expr);
            } else {
                break;
            }
        }
        return expr;
    }
    
    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();
        // This check allows for 0 arguments by not populating the args list.
        if (!matchCurrentToken(RIGHT_PAREN)) {
            // Section 10.1.1: C requires implementations support at least 127,
            //                 Java requires no more than 255 arguments.
            if (arguments.size() >= 255) {
                logParseError(peekCurrentToken(), "Can't have more than 255 arguments.");
            }
            // Retrieve a list of all this function call's arguments.
            do {
                arguments.add(parseExpression());
            } // Very important to consume the comma to proceed to the next arg!
            while (consumeTokenIfMatchesAny(OPERATOR_COMMA));
        }
        Token paren = consumeTokenOrThrow(RIGHT_PAREN, "Expected ')' after arguments.");
        return new Expr.Call(callee, paren, arguments);
    }

    /**
     * primary -> NUMBER_LITERAL | STRING_LITERAL 
     *          | "true" 
     *          | "false" 
     *          | "nil"
     *          | "(" expression ")" 
     *          | expression ( "," expression )+ ; 
     */
    private Expr parsePrimary() {
        if (consumeTokenIfMatchesAny(KEYWORD_FALSE)) {
            return new Expr.Literal(false);
        }
        if (consumeTokenIfMatchesAny(KEYWORD_TRUE)) {
            return new Expr.Literal(true);
        }
        if (consumeTokenIfMatchesAny(KEYWORD_NIL)) {
            return new Expr.Literal(null);
        }
        // Our scanner/lexer already retrieved the in-memory object of this.
        if (consumeTokenIfMatchesAny(NUMBER_LITERAL, STRING_LITERAL)) {
            return new Expr.Literal(peekPreviousToken().literal);
        }
        // We just parsed a variable name or identifier.
        if (consumeTokenIfMatchesAny(IDENTIFIER)) {
            return new Expr.Variable(peekPreviousToken());
        }
        if (consumeTokenIfMatchesAny(LEFT_PAREN)) {
            Expr expr = parseExpression();
            consumeTokenOrThrow(RIGHT_PAREN, "Expected ')' after expression.");
            return new Expr.Grouping(expr);
        }

        // We have a token that can't start an expression.
        Token token = peekCurrentToken();
        switch (token.type) {
            // Allow for unary minus though
            case OPERATOR_ADD:
            case OPERATOR_DIV:
            case OPERATOR_MUL:
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
    // private Expr parseCommaOrExpr(Object literal) {
    //     if (consumeTokenIfMatchesAny(OPERATOR_COMMA)) {
    //         // Left expression and operand already updated counter propeerly
    //         return parseExpression();            
    //     }
    //     return new Expr.Literal(literal);
    // }
    
    
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
            if (peekPreviousToken().type == OPERATOR_SEMI) {
                return;
            }
            // Any of the labelled tokens indicate we're about to start a statement.
            // For the rest, we just keep consuming tokens.
            switch (peekCurrentToken().type) {
                case KEYWORD_CLASS:
                case KEYWORD_FUN:
                case KEYWORD_VAR:
                case KEYWORD_IF:
                case KEYWORD_FOR:
                case KEYWORD_WHILE:
                // case KEYWORD_PRINT:
                case KEYWORD_RETURN: return;
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
        return peekCurrentToken().type == END_OF_FILE;
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
