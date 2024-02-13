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
    private final Environment closure; // Hold on to any external local variables!
    private final boolean isInitializer;
                                       
    LoxFunction(Stmt.Function declaration, Environment closure, boolean isInitializer) {
        this.declaration = declaration;
        this.closure = closure;
        this.isInitializer = isInitializer;
    }
    
    /**
     * Create a new environment nestled in the method's original closure so that
     * when the method is called, it becomes the parent of the method body's
     * environment.
     * 
     * So this creates a little persistence environment where "this" is bound to
     * the object instance.
     */
    LoxFunction bind(LoxInstance instance) {
        Environment environment = new Environment(this.closure);
        environment.defineVariable("this", instance);
        return new LoxFunction(declaration, environment, this.isInitializer);
    }
    
    /* Assumes that we verified the user supplied the correct number of arguments. */
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        // Create a new, localized, inner environment which goes out from the
        // function's body all the way out until the global scope.
        Environment environment = new Environment(closure);

        // Populate the inner environment with the parameter names and their values.
        for (int i = 0; i < this.declaration.params.size(); i++) {
            String ident = this.declaration.params.get(i).lexeme;
            environment.defineVariable(ident, arguments.get(i));
        }
        
        try {
            // The function body expects the exact parameter names to appear.
            // After this, the local environment will be discarded.
            interpreter.executeBlock(this.declaration.body, environment);
        } catch (Return returnValue) {
            // Allow early void returns in class initializer functions.
            // This will instead return the 'this' object.
            if (this.isInitializer) {
                return closure.retrieveLocalVariable(0, "this");
            }
            return returnValue.value;
        }
        // For initializer functions, try to forcibly return 'this'.
        if (this.isInitializer) {
            return this.closure.retrieveLocalVariable(0, "this");
        }
        // If no 'return' was found, we default to returning a nil value.
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
