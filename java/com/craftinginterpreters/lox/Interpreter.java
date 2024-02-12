package com.craftinginterpreters.lox;

import java.util.List;

/**
 * We'll use Java's Object type to represent all of our Lox types. 
 * We'll then query each object instance for their specific type.
 * 
 * Lox:     | nil  | boolean | number | string |
 * Java:    | null | Boolean | Double | String |
 * 
 * Expressions produce values but statements do not.
 */
class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    // Store all our variables as long as this interpreter instance runs.
    private Environment environment = new Environment();
    
    /* Take a syntax tree for an expression then try to evaluate it. */
    void interpret(List<Stmt> statements) {
        try {
            for (Stmt statement : statements) {
                executeStatement(statement);
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    /* Literals are *almost* values. This is the parser's domain. */
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value; // Was produced during the scanning/lexing phase.
    }
    
    @Override
    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = evaluateExpression(expr.left);

        // These checks actually implement the "short circuiting" behavior.
        if (expr.operator.type == TokenType.KEYWORD_OR) {
            if (isTruthy(left)) {
                return left;
            }
        } else {
            if (!isTruthy(left)) {
                return left;
            }
        }
        
        return evaluateExpression(expr.right);
    }
    
    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluateExpression(expr.right);
        switch (expr.operator.type) {
            case OPERATOR_NOT:
                return !isTruthy(right);
            case OPERATOR_SUB: 
                checkNumberOperand(expr.operator, right);
                return -(double)right; // This cast may fail!
            default: 
                break;
        }
        // Unreachable since we only allow unary minus and unary not.
        return null;
    }
    
    @Override
    public Object visitVariableExpr(Expr.Variable expr) {
        return environment.retrieveVariable(expr.name);
    }
    
    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) {
            return;
        }
        throw new RuntimeError(operator, "Operand must be a number.");
    }
    
    /* Following languages like Ruby and Lua: false and nil are falsy. */
    private boolean isTruthy(Object object) {
        if (object == null) {
            return false;
        } 
        if (object instanceof Boolean) {
            return (boolean)object;
        }
        return true;
    }
    
    private boolean isEqual(Object a, Object b) {
        if (a == null && b == null) {
            return true;
        }
        if (a == null) {
            return false;
        }
        return a.equals(b);
    }
    
    private String stringifyObject(Object object) {
        if (object == null) {
            return "nil";
        }
        if (object instanceof Double) {
            String text = object.toString();
            // Trim ".0" if we can represent a pure integer.
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }
        return object.toString();
    }
    
    /** 
     * A grouping node has an inner reference to an inner node for the expr
     * contained inside the parens. We have to recursively evalue subexprs.
     */
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluateExpression(expr.expression);
    }
    
    /**
     * Ends up "deferring" to one of the "specializations" in Expr.java, which
     * in turn ends up calling one of the Expr.*.visit*Expr methods which we
     * defined above.
     */
    private Object evaluateExpression(Expr expr) {
        return expr.accept(this);
    }
    
    /** 
     * Ends up "deferring" to one of the "specializations" in Stmt.java, which
     * in turn ends up calling one of the Stmt.*.visit*Stmt methods which we 
     * defined above.
     */
    private void executeStatement(Stmt stmt) {
        stmt.accept(this);
    }
    
    void executeBlock(List<Stmt> statements, Environment environment) {
        // Save the current environment so we can return to it.
        Environment previous = this.environment;
        // Execute all of the inner block's statements until all done.
        try {
            this.environment = environment;
            for (Stmt statement : statements) {
                executeStatement(statement);
            }
        } finally {
            this.environment = previous;
        }
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        executeBlock(stmt.statements, new Environment(environment));
        return null;
    }
    
    /* Evaluate then discard the value. */
    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        evaluateExpression(stmt.expression);
        return null;
    }
    
    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        if (isTruthy(evaluateExpression(stmt.condition))) {
            executeStatement(stmt.thenBranch);
        } else if (stmt.elseBranch != null) {
            executeStatement(stmt.elseBranch);
        }
        return null;
    }
    
    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        Object value = evaluateExpression(stmt.expression);
        System.out.println(stringifyObject(value));
        return null;
    }
    
    /* Lack of an initializer is NOT a syntax error for Lox. */
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = null;
        if (stmt.initializer != null) {
            value = evaluateExpression(stmt.initializer);
        }
        environment.defineVariable(stmt.name.lexeme, value);
        return null;
    } 
    
    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        while (isTruthy(evaluateExpression(stmt.condition))) {
            executeStatement(stmt.body);
        }
        return null;
    }
    
    /** 
     * Lox does not allow implicit variable declarations, but allows nested  
     * expressions when assigning to a SINGLE variable.
     */
    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        Object value = evaluateExpression(expr.value);
        environment.assignVariable(expr.name, value);
        return value;
    }
    
    /* Recursively evaluate subexpressions until we're left with 2. */
    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluateExpression(expr.left);
        Object right = evaluateExpression(expr.right);

        switch(expr.operator.type) {
            case OPERATOR_ADD:
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                }
                // We're effectively overloading the '+' operator here.
                if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }
                // If both operands aren't the same type, too bad!
                throw new RuntimeError(expr.operator, 
                    "Operands must be two numbers or two strings.");
            case OPERATOR_SUB:
                checkNumberOperands(expr.operator, left, right);
                return (double)left - (double)right;
            case OPERATOR_DIV:
                checkNumberOperands(expr.operator, left, right);
                return (double)left / (double)right;
            case OPERATOR_MUL:
                checkNumberOperands(expr.operator, left, right);
                return (double)left * (double)right;
            case OPERATOR_GT:
                checkNumberOperands(expr.operator, left, right);
                return (double)left > (double)right;
            case OPERATOR_GE:
                checkNumberOperands(expr.operator, left, right);
                return (double)left >= (double)right;
            case OPERATOR_LT:
                checkNumberOperands(expr.operator, left, right);
                return (double)left < (double)right;
            case OPERATOR_LE:
                checkNumberOperands(expr.operator, left, right);
                return (double)left <= (double)right;
            case OPERATOR_EQ:
                checkNumberOperands(expr.operator, left, right);
                return isEqual(left, right);
            case OPERATOR_NEQ:
                checkNumberOperands(expr.operator, left, right);
                return !isEqual(left, right);
            default:
                break;
        }
        // Unreachable unless you do something really cursed.
        return null;
    }

    private void checkNumberOperands(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double) {
            return;
        }
        throw new RuntimeError(operator, "Operands must be numbers.");
    }
}
