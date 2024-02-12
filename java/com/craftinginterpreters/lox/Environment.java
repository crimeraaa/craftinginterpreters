package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

/**
 * We'll store Lox variable names and their values in a key-value table. 
 * We use strings to allow for lookup since tokens are only in reference to 
 * source code.
 * 
 * For now this is all in the global scope.
 */
class Environment {
    final Environment enclosing; // Used to create local scopes.
    private final Map<String, Object> values = new HashMap<>(); 
    
    /* No-arg constructor is for global environment. */
    Environment() {
        enclosing = null;
    }
    
    /* 1 arg constructor creates a new local scope based on the outer one. */
    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }
    
    /** 
     * Define a variable, with a value (or null) in the CURRENT scope.
     * This allows redefining variable with the var keyword as well. 
     */
    void defineVariable(String name, Object value) {
        this.values.put(name, value);
    }
    
    /**
     * Use the results of our static analysis to retrieve a local variable.
     */
    Object retrieveLocalVariable(int distance, String name) {
        return retrieveEnclosingEnvironment(distance).values.get(name);
    }
    
    /**
     * Retrieve an enclosing environment which we are sure contains a variable
     * we are looking for.
     */
    Environment retrieveEnclosingEnvironment(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }
        return environment;
    }
    
    /**
     * Walk the chain of enclosing environments to find some global variable.
     * For local variables or external local variables, you need to determine
     * the number of scopes away your desired variable is from your current scope
     * then use retrieveLocalVariable.
     */
    Object retrieveGlobalVariable(Token name) {
        String ident = name.lexeme;
        // If this variable shadowed an enclosing name we'll prefer the local.
        if (this.values.containsKey(ident)) {
            return this.values.get(ident);
        }
        // No local variable of this name so try to get an enclosing one.
        if (this.enclosing != null) {
            return this.enclosing.retrieveGlobalVariable(name);
        }
        throw new RuntimeError(name, "Undefined variable '" + ident + "'.");
    }
    
    /* Assign a new value to an already existing variable. */
    void assignGlobalVariable(Token name, Object value) {
        String ident = name.lexeme;
        if (this.values.containsKey(ident)) {
            this.values.put(ident, value);
            return;
        }
        if (this.enclosing != null) {
            this.enclosing.assignGlobalVariable(name, value);
            return;
        }
        throw new RuntimeError(name, "Undefined variable '" + ident + "'.");
    } 
    
    void assignLocalVariable(int distance, Token name, Object value) {
        retrieveEnclosingEnvironment(distance).values.put(name.lexeme, value);
    }
}
