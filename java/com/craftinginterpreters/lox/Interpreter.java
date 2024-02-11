package com.craftinginterpreters.lox;

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
    /* Take a syntax tree for an expression then try to evaluate it. */
    void interpret(Expr expression) {
        try {
            Object value = evaluateExpr(expression);
            System.out.println(stringifyValue(value));
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
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluateExpr(expr.right);
        switch (expr.operator.type) {
            case TK_COMPARE_NOT:
                return !isTruthy(right);
            case TK_MINUS: 
                checkNumberOperand(expr.operator, right);
                return -(double)right; // This cast may fail!
            default: 
                break;
        }
        // Unreachable since we only allow unary minus and unary not.
        return null;
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
    
    private String stringifyValue(Object object) {
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
        return evaluateExpr(expr.expression);
    }
    
    private Object evaluateExpr(Expr expr) {
        return expr.accept(this);
    }
    
    /* Evaluate then discard the value. */
    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        evaluateExpr(stmt.expression);
        return null;
    }
    
    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        Object value = evaluateExpr(stmt.expression);
        System.out.println(stringifyValue(value));
        return null;
    }
    
    /* Recursively evaluate subexpressions until we're left with 2. */
    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluateExpr(expr.left);
        Object right = evaluateExpr(expr.right);

        switch(expr.operator.type) {
            case TK_PLUS:
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
            case TK_MINUS:
                checkNumberOperands(expr.operator, left, right);
                return (double)left - (double)right;
            case TK_SLASH:
                checkNumberOperands(expr.operator, left, right);
                return (double)left / (double)right;
            case TK_STAR:
                checkNumberOperands(expr.operator, left, right);
                return (double)left * (double)right;
            case TK_COMPARE_GT:
                checkNumberOperands(expr.operator, left, right);
                return (double)left > (double)right;
            case TK_COMPARE_GE:
                checkNumberOperands(expr.operator, left, right);
                return (double)left >= (double)right;
            case TK_COMPARE_LT:
                checkNumberOperands(expr.operator, left, right);
                return (double)left < (double)right;
            case TK_COMPARE_LE:
                checkNumberOperands(expr.operator, left, right);
                return (double)left <= (double)right;
            case TK_COMPARE_EQ:
                checkNumberOperands(expr.operator, left, right);
                return isEqual(left, right);
            case TK_COMPARE_NEQ:
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
