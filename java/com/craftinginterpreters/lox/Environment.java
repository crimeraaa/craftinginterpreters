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
    private final Map<String, Object> values = new HashMap<>(); 
    
    /* This allows redefining variable with the var keyword as well. */
    void defineVariable(String name, Object value) {
        values.put(name, value);
    }
    
    Object retrieveVariable(Token name) {
        String ident = name.lexeme;
        if (values.containsKey(ident)) {
            return values.get(ident);
        }
        throw new RuntimeError(name, "Undefined variable '" + ident + "'.");
    }
    
    /* Assign a new value to an already existing variable. */
    void assignVariable(Token name, Object value) {
        String ident = name.lexeme;
        if (values.containsKey(ident)) {
            values.put(ident, value);
            return;
        }
        throw new RuntimeError(name, "Undefined variable '" + ident + "'.");
    }
}
