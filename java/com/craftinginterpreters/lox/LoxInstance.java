package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
    private LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }
    
    @Override
    public String toString() {
        return klass.name + " instance";
    }
    
    /**
     * Fields are bits of state stored directly by an instance.
     * Properties are named things that an access may return, but not all
     * properties are fields. However, all fields are properties.
     *
     * This is because properties are owned by the class definition, not any one
     * instance.
     */
    Object accessProperty(Token name) {
        if (this.fields.containsKey(name.lexeme)) {
            return this.fields.get(name.lexeme);
        }
        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null) {
            return method.bind(this);
        }
        throw new RuntimeError(name, "Undefined property '" + name.lexeme + "'");
    }
    
    /* May create new keys! */
    void set(Token name, Object value) {
        this.fields.put(name.lexeme, value);
    }
}
