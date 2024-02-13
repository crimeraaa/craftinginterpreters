package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.List;

/** 
 * Use exceptions to unwind a heavily recursive tree-walk interpreter. 
 * It's not pretty but for our purposes it works.
 * This also helps us escape out of deeply nested functions.
 */
class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        // Disable JVM magic we don't need, like stack traces
        super(null, null, false, false);
        this.value = value;
    }
}

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
    // Fixed references to the outermost global environment.
    final Environment globals = new Environment();

    // Track changes as we enter and exit local scopes: the current environment.
    private Environment environment = globals;

    // Associate each syntax tree node with its resolved data.
    private final Map<Expr, Integer> locals = new HashMap<>();
    
    Interpreter() {
        // Creates a global builtin function called clock().
        this.globals.defineVariable("clock", new LoxCallable() {
            @Override
            public int arity() {
                return 0;
            }

            @Override 
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double)System.currentTimeMillis() / 1000.0;
            }
            
            @Override
            public String toString() {
                return "<native fn>";
            }
        });
        // Instead of a keyword, let's make it a function.
        this.globals.defineVariable("print", new LoxCallable() {
            @Override
            public int arity() {
                return 1;
            }
            
            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                for (Object argument : arguments) {
                    System.out.println(stringifyObject(argument));
                }
                return null;
            }
            
            @Override
            public String toString() {
                return "<native fn>";
            }
        });
        // Based off of Lua's `type` builtin function.
        this.globals.defineVariable("type", new LoxCallable() {
            @Override
            public int arity() {
                return 1;
            }
            
            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                for (Object argument : arguments) {
                    if (argument == null) {
                        return "nil";
                    }
                    return argument.getClass()
                        .toString()
                        .replaceFirst("class java.lang.", "")
                        .toLowerCase();
                }
                return null;
            }

            @Override
            public String toString() {
                return "<native fn>";
            }
        });
    }
    
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
    public Object visitSetExpr(Expr.Set expr) {
        Object object = evaluateExpression(expr.object);
        if (!(object instanceof LoxInstance)) {
            throw new RuntimeError(expr.name, "Only instances have fields.");
        }
        Object value = evaluateExpression(expr.value);
        ((LoxInstance)object).set(expr.name, value);
        return value;
    }
    
    @Override
    public Object visitSuperExpr(Expr.Super expr) {
        int distance = this.locals.get(expr);
        LoxClass superclass = (LoxClass)this.environment.retrieveLocalVariable(
            distance, "super");
        // Even though we don't have a convenient node to hang on to for 'this',
        // we control the layout of the environment chains so 'this' is always
        // right inside of the environment where we store 'super'.
        // We assume that 'this' is in an inner environment, hence - 1.
        LoxInstance object = (LoxInstance)this.environment.retrieveLocalVariable(
            distance - 1, "this");
        LoxFunction method = superclass.findMethod(expr.method.lexeme);
        if (method == null) {
            throw new RuntimeError(expr.method, 
                "Undefined property '" + expr.method.lexeme + "'.");
        }
        return method.bind(object);
    }
    
    @Override
    public Object visitThisExpr(Expr.This expr) {
        return lookUpVariable(expr.keyword, expr);
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
        // return environment.retrieveVariable(expr.name);
        return lookUpVariable(expr.name, expr); // Changed in Chaper 11.4.1
    }
    
    /** 
     * Look up only resolved distance in the map, which contains only locals.
     * Remember that globals are treated specially and don't end up in the map.
     */
    private Object lookUpVariable(Token name, Expr expr) {
        Integer distance = this.locals.get(expr);
        if (distance != null) {
            return this.environment.retrieveLocalVariable(distance, name.lexeme);
        } else {
            return this.globals.retrieveGlobalVariable(name);
        }
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
    
    /**
     * Every time our resolver visits a variable, it tells the interpeter
     * how many scopes are there between the current scope and the scope where
     * the variable is defined.
     * 
     * At runtime this corresponds to exactly the number of environments between
     * the current one and the enclosing one where the interpreter can find the
     * variable's value. This function simply hands that number to the interpreter.
     */
    void resolve(Expr expr, int depth) {
        this.locals.put(expr, depth);
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
    
    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        Object superclass = null;
        if (stmt.superclass != null) {
            superclass = evaluateExpression(stmt.superclass);
            // Disallow inheritance of non-class types because it's unreasonable
            if (!(superclass instanceof LoxClass)) {
                throw new RuntimeError(stmt.superclass.name, 
                    "Superclass must be a class.");
            }
        }
        this.environment.defineVariable(stmt.name.lexeme, null);
        
        // When we evaluate a subclass definition, we create a new environment.
        // In this environment we store a reference to the superclass, which is 
        // the actual LoxClass object for the superclass at runtime.
        if (stmt.superclass != null) {
            this.environment = new Environment(this.environment);
            this.environment.defineVariable("super", superclass);
        }
        
        // Turn syntactic representation of the class (the AST node) into its
        // runtime representation.
        Map<String, LoxFunction> methods = new HashMap<>();
        for (Stmt.Function method : stmt.methods) {
            boolean isInitializer = method.name.lexeme.equals("init");
            LoxFunction function = new LoxFunction(method, environment, isInitializer);
            methods.put(method.name.lexeme, function);
        }

        LoxClass klass = new LoxClass(stmt.name.lexeme, (LoxClass)superclass, methods);

        // Pop the inner superclass environment if we made one.
        if (superclass != null) {
            this.environment = this.environment.enclosing;
        }
        this.environment.assignGlobalVariable(stmt.name, klass);
        return null;
    }
    
    /* Evaluate then discard the value. */
    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        evaluateExpression(stmt.expression);
        return null;
    }
    
    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        // Create a function syntax node, or the compile-time representation.
        // Also capture the current environment, especially for closures.
        // However this is the environment for when the function is declared,
        // not the environment when it's called.
        // Also, in case the user defined a non-class method function literally 
        // called 'init', we shouldn't treat that as an initializer.
        LoxFunction function = new LoxFunction(stmt, this.environment, false);
        // Create the runtime representation of our function.
        this.environment.defineVariable(stmt.name.lexeme, function);
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
    
    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = null;
        if (stmt.value != null) {
            value = evaluateExpression(stmt.value);
        }
        // Use exceptions to unwind call stack back to caller
        throw new Return(value); 
    }
    
    /* Lack of an initializer is NOT a syntax error for Lox. */
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = null;
        if (stmt.initializer != null) {
            value = evaluateExpression(stmt.initializer);
        }
        this.environment.defineVariable(stmt.name.lexeme, value);
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
        Integer distance = this.locals.get(expr);
        if (distance != null) {
            this.environment.assignLocalVariable(distance, expr.name, value);
        } else {
            this.globals.assignGlobalVariable(expr.name, value);
        }
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
    
    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = evaluateExpression(expr.callee);
        List<Object> arguments = new ArrayList<>();
        for (Expr argument : expr.arguments) {
            arguments.add(evaluateExpression(argument));
        }
        if (!(callee instanceof LoxCallable)) {
            throw new RuntimeError(expr.paren, "Can only call functions and classes.");
        }
        LoxCallable function = (LoxCallable)callee;
        if (arguments.size() != function.arity()) {
            throw new RuntimeError(expr.paren, 
                "Expected " + function.arity() + " arguments but got " + arguments.size() + ".");
        }
        return function.call(this, arguments);
    }
    
    @Override
    public Object visitGetExpr(Expr.Get expr) {
        Object object = evaluateExpression(expr.object);
        if (object instanceof LoxInstance) {
            return ((LoxInstance) object).accessProperty(expr.name);
        }
        throw new RuntimeError(expr.name, "Only instances have properties.");
    }

    private void checkNumberOperands(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double) {
            return;
        }
        throw new RuntimeError(operator, "Operands must be numbers.");
    }
}
