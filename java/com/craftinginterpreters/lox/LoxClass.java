package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements LoxCallable {
    final String name;
    private final Map<String, LoxFunction> methods;

    LoxClass(String name, Map<String, LoxFunction> methods) {
        this.name = name;
        this.methods = methods;
    }
    
    LoxFunction findMethod(String name) {
        if (this.methods.containsKey(name)) {
            return this.methods.get(name);
        }
        return null;
    }
    
    @Override
    public String toString() {
        return "class " + name;
    }
    
    /**
     * Calling the class as a function, C++ style, "constructs" a new instance.
     * Unlike C++ we don't take constructor arguments (or not yet anyway).
     */
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod("init");
        // Everytime a class is constructed, try to call the default initializer.
        // The argument list will be forwarded.
        if (initializer != null) {
            initializer.bind(instance).call(interpreter, arguments);
        }
        return instance;
    }
    
    /**
     * If there is an initializer, use its arity. Otherwise, since we don't 
     * require an initializer to be present, we default to 0.
     */
    @Override
    public int arity() {
        LoxFunction initializer = findMethod("init");
        if (initializer == null) {
            return 0;
        }
        return initializer.arity();
    }
}
