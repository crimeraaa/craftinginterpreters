// https://craftinginterpreters.com/resolving-and-binding.html#a-resolver-class
package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

/**
 * Visits every node in our syntax tree and does static analysis in order to
 * resolve variables, particuarly external local variables captured by closures
 * that are shadowed later on in a particular scope.
 * 
 * It is only concerned with:
 * resolve      -> block        // Introduces new scope for the contained statements.
 *               | function     // Introduce new scope for body, bind parameters.
 *               | varDecl      // Adds a new variable to the current scope.
 *               | assignment ; // Variables must be resolved, not necessarily values.
 *               
 * For the rest, they are visited but nothing is done with them.
 */
class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private enum FunctionType {
        NONE,
        FUNCTION,
        INITIALIZER,
        METHOD,
    }
    
    private enum ClassType {
        NONE,
        CLASS
    }
    
    private ClassType currentClass = ClassType.NONE;

    private final Interpreter interpreter;
    /** 
     * Lexcial scopes nest in both interpreter and resolver, like a stack. 
     * We keep track of stack of scopes for the current scope.
     * 
     * Keys are variable names and values are booleans for...
     * 
     * This is only used for local block scopes. Variables declares at the top
     * level in the global scope are not tracked by the resolver since they are
     * more dynamic in Lox. If, when resolving a variable, we can't find it in 
     * the stack of local scopes, we assume it's a global.
     */
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();

    // Helps us track invalid return statements like in the top-level scope.
    private FunctionType currentFunction = FunctionType.NONE;

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }
    
    /* Begin a new scope, traverse into its statements, then discard the scope. */
    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginBlockScope();
        resolveBlockStmts(stmt.statements);
        endBlockScope();
        return null;
    } 
    
    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        ClassType enclosingClass = this.currentClass;
        currentClass = ClassType.CLASS;

        declareVar(stmt.name);
        defineVar(stmt.name);
        
        // Create a closure for the implicit 'this' variable.
        // This allows us to resolve it to a local variable within methods.
        beginBlockScope();
        this.scopes.peek().put("this", true);
        for (Stmt.Function method : stmt.methods) {
            FunctionType declaration = FunctionType.METHOD;
            // May be a user-defined non-method function called 'init', so use
            // visitReturnStmt in Resolver to double check.
            if (method.name.lexeme.equals("init")) {
                declaration = FunctionType.INITIALIZER;
            }
            resolveFunction(method, declaration);
        }
        endBlockScope();
        this.currentClass = enclosingClass;
        return null;
    }
    
    /* An expression statement contains a single expression to traverse. */
    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolveExpr(stmt.expression);
        return null;
    }
    
    /** 
     * An if statement has an expression for its condition and 1/2 statements 
     * for its branches.
     * 
     * Resolution is different from interpretation! Here, there is no control flow.
     * We resolve the condition and both branches, analyzing all of them.
     */
    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        resolveExpr(stmt.condition);
        resolveStmt(stmt.thenBranch);
        if (stmt.elseBranch != null) {
            resolveStmt(stmt.elseBranch);
        }
        return null;
    }
    
    /* Unused since I turned print into a function, but need for completeness. */
    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolveExpr(stmt.expression);
        return null;
    }
    
    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (this.currentFunction == FunctionType.NONE) {
            Lox.error(stmt.keyword, "Can't return from top-level code!");
        }
        if (stmt.value != null) {
            if (this.currentFunction == FunctionType.INITIALIZER) {
                Lox.error(stmt.keyword, "Can't return a value from an initializer.");
            }
            resolveExpr(stmt.value);
        }
        return null;
    }
    
    /* Similar to an if, we need to resolve both the condition and the body. */
    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        resolveExpr(stmt.condition);
        resolveStmt(stmt.body);
        return null;
    }
    
    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolveExpr(expr.left);
        resolveExpr(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolveExpr(expr.callee); // May be a variable expression, so resolve it!
        for (Expr argument : expr.arguments) {
            resolveExpr(argument);
        }
        return null;
    }
    
    /** 
     * Properties of classes are looked up dynamically. 
     * This means they aren't resolved by the static analysis.
     */
    @Override
    public Void visitGetExpr(Expr.Get expr) {
        resolveExpr(expr.object);
        return null;
    }
    
    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolveExpr(expr.expression);
        return null;
    }
    
    /* Literal expressions don't mention any variables so we do nothing. */
    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        return null;
    }

    /**
     * Static analysis does not control flow nor short circuiting.
     * Because of this we evaluate all expressions in and's and or's.
     */
    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolveExpr(expr.left);
        resolveExpr(expr.right);
        return null;
    }
    
    @Override
    public Void visitSetExpr(Expr.Set expr) {
        resolveExpr(expr.value);
        resolveExpr(expr.object);
        return null;
    }
    
    @Override
    public Void visitThisExpr(Expr.This expr) {
        if (this.currentClass == ClassType.NONE) {
            Lox.error(expr.keyword, "Can't use 'this' outside of a class.");
        }
        resolveLocal(expr, expr.keyword);
        return null;
    }
    
    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolveExpr(expr.right);
        return null;
    }

    /**
     * Remember that functions both bind names AND introduce a scope. 
     * The function name is bound in the surrounding scope where the function is
     * declared. In the function's body, we also bind its parameters.
     */
    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        declareVar(stmt.name);
        defineVar(stmt.name);
        // Name is defined before function body to allow recursion.
        resolveFunction(stmt, FunctionType.FUNCTION);
        return null;
    }
    
    /* Split binding into two steps, throw compile error on variable shadowing. */
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        declareVar(stmt.name);
        if (stmt.initializer != null) {
            resolveExpr(stmt.initializer);
        }
        defineVar(stmt.name);
        return null;
    }
    
    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolveExpr(expr.value); // Resolve references to other variables.
        resolveLocal(expr, expr.name); // Resolve the variable being assigned to.
        return null;
    }
    
    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!this.scopes.isEmpty() && this.scopes.peek().get(expr.name.lexeme) == Boolean.FALSE) {
            Lox.error(expr.name, "Can't read local variable in its own initializer.");
        }
        resolveLocal(expr, expr.name);
        return null;
    }
    
    /** 
     * Add variable to innermost scope so that it shadows any outer one. 
     * We use false to mark it as "not ready yet" or "not yet finished resolving".
     */
    private void declareVar(Token name) {
        if (this.scopes.isEmpty()) {
            return;
        }
        // I guess this is like .pop without actually popping *off* the stack?
        Map<String, Boolean> scope = this.scopes.peek();

        // Redeclaration of local variables in the same scope is likely an error.
        // We do allow this in the global scope though.
        // TODO: Do allow `var a = "Global"; { var a = a; print(a); }` though?
        if (scope.containsKey(name.lexeme)) {
            Lox.error(name, "Already a variable with this name in the scope.");
        }
        scope.put(name.lexeme, false);
    }
    
    /**
     * After declaring a variable we resolve its initializer expression in that
     * same scope where the new variable exists but is not unavailable.
     * We mark it as true to indicate it's ready to be used.
     */
    private void defineVar(Token name) {
        if (this.scopes.isEmpty()) {
            return;
        }
        this.scopes.peek().put(name.lexeme, true);
    }
    
    private void resolveLocal(Expr expr, Token name) {
        // Start an innermost scope, going outwards.
        for (int i = this.scopes.size() - 1; i >= 0; i--) {
            // If we find the variable, resolve it using the number of hops it
            // took to resolve it. Current scope = 0, immediate enclosing = 1, etc.
            if (this.scopes.get(i).containsKey(name.lexeme)) {
                this.interpreter.resolve(expr, this.scopes.size() - 1 - i);
                return;
            }
        }
        // If not found, assume the variable is global.
    }
    
    void resolveBlockStmts(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolveStmt(statement);
        }
    }
    
    /* Defer to any of the visit*Stmt methods we defined above. */
    private void resolveStmt(Stmt stmt) {
        stmt.accept(this);
    }
    
    /* Defer to any of the `visit*Expr` methods we defined above. */
    private void resolveExpr(Expr expr) {
        expr.accept(this);
    }
    
    /* Create a new scope for function body then bind variables to parameters. */
    private void resolveFunction(Stmt.Function function, FunctionType type) {
        // Hold this so we can restore state afterwards.
        FunctionType enclosingFunction = this.currentFunction;
        this.currentFunction = type;

        beginBlockScope();
        for (Token param : function.params) {
            declareVar(param);
            defineVar(param);
        }
        resolveBlockStmts(function.body);
        endBlockScope();
        this.currentFunction = enclosingFunction;
    }
    
    private void beginBlockScope() {
        this.scopes.push(new HashMap<String, Boolean>());
    }
    
    private void endBlockScope() {
        this.scopes.pop();
    }
}
