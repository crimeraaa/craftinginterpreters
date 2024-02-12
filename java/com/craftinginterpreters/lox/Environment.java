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
    
    Object retrieveVariable(Token name) {
        String ident = name.lexeme;
        // If this variable shadowed an enclosing name we'll prefer the local.
        if (this.values.containsKey(ident)) {
            return this.values.get(ident);
        }
        // No local variable of this name so try to get an enclosing one.
        if (this.enclosing != null) {
            return this.enclosing.retrieveVariable(name);
        }
        throw new RuntimeError(name, "Undefined variable '" + ident + "'.");
    }
    
    /* Assign a new value to an already existing variable. */
    void assignVariable(Token name, Object value) {
        String ident = name.lexeme;
        if (this.values.containsKey(ident)) {
            this.values.put(ident, value);
            return;
        }
        if (this.enclosing != null) {
            this.enclosing.assignVariable(name, value);
            return;
        }
        throw new RuntimeError(name, "Undefined variable '" + ident + "'.");
    }
}
