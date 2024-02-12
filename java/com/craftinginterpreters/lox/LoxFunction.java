package com.craftinginterpreters.lox;

import java.util.List;

/* This is a sort of "template" for Lox functions. See below. */
interface LoxCallable {
    int arity(); // Exact number of arguments this function must take. No more, no less.
    Object call(Interpreter interpreter, List<Object> arguments);
}

/** 
 * This is how builtin and user-defined functions are actually created in-memory. 
 * When creating a builtin function (that is, from Java) we need to provide
 * arity() and call() for each function definition we want, e.g. see clock().
 */
class LoxFunction implements LoxCallable {
    private final Stmt.Function declaration;
    LoxFunction(Stmt.Function declaration) {
        this.declaration = declaration;
    }
    
    /* Assumes that we verified the user supplied the correct number of arguments. */
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        // Create a new, localized, inner environment.
        Environment environment = new Environment(interpreter.globals);
        // Populate the inner environment with the parameter names and their values.
        for (int i = 0; i < this.declaration.params.size(); i++) {
            String ident = this.declaration.params.get(i).lexeme;
            environment.defineVariable(ident, arguments.get(i));
        }
        // The function body expects the exact parameter names to appear.
        // After this, the local environment will be discarded.
        interpreter.executeBlock(this.declaration.body, environment);
        return null;
    }

    @Override
    public int arity() {
        return this.declaration.params.size();
    }
    
    @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }
}
