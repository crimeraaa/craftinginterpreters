package com.craftinginterpreters.lox;

class AstPrinter implements Expr.Visitor<String> {
    String print(Expr expr) {
        return expr.accept(this); // Recursion lets us print an entire tree!
    }
    
    @Override
    public String visitVariableExpr(Expr.Variable expr) {
        return parenthesize(expr.name.lexeme, expr);
    }
    
    @Override
    public String visitAssignExpr(Expr.Assign expr) {
        return parenthesize(expr.name.lexeme, expr);
    }
    
    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }
    
    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }
    
    /* Converting literals to strings is easy! */
    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) {
            return "nil";
        }
        return expr.value.toString();
    }
    
    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }
    
    private String parenthesize(String name, Expr ...exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this)); // Gonna be recursive!!
        }
        builder.append(")");

        return builder.toString();
    }
    
    /**
     * Compile:     `javac --class-path <path-before-com> -d <path-before-com>`
     * Run:         `java --class-path <path-before-com> com/craftinginterpreters/lox/AstPrinter`
     */
    public static void main(String[] args) {
        Expr expression = new Expr.Binary(
            new Expr.Unary(
                new Token(TokenType.TK_MINUS, "-", null, 1),
                new Expr.Literal(123)),
            new Token(TokenType.TK_STAR, "*", null, 1), 
            new Expr.Grouping(new Expr.Literal(45.67)));
        
        System.out.println(new AstPrinter().print(expression));
    }
}
