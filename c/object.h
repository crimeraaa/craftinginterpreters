#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"

/* LoxObject tag. */
typedef enum {
    OBJ_STRING,
} LoxObjectType;

/** 
 * LoxObject struct proper. This the "base" class (for lack of better word) for
 * all of our non-trivial types/objects.
 */
struct LoxObject {
    LoxObjectType type; 
    LoxObject *next; // We use an instrusive linked list to track allocations.
};

/**
 * LoxString struct proper. This is our internal representation of user-facing
 * strings.
 */
struct LoxString {
    LoxObject obj; // Header to mimic inheritance via type punning.
    int length; // Number of characters in the buffer save for the nul terminator.
    char *buffer; // Dynamically allocated string buffer.
    uint32_t hash; // Hash code result of hash function. Calculated once.
};

/**
 * Creates a heap-allocated copy of `literal` and creates a new LoxString for it.
 * If you don't want a copy of `literal`, consider using `take_string()`.
 */
LoxString *copy_string(const char *literal, int length);

/**
 * Assumes `buffer` was allocated elsewhere so we just hold it in a LoxString.
 * Mainly used with `vm.c:concatenate()`.
 */
LoxString *take_string(char *buffer, int length);

/**
 * Due to the fact that LoxObjects can be anything, for the builtin ones we 
 * already specify operations on how to print them to the console.
 */
void print_object(LoxValue value);

/**
 * We can't use a macro because the value is used in two places. If value were
 * a macro that had side effects, like pop(), you'd probably pop a value off the
 * VM's stack twice which is probably not what you want!
 */
static inline bool isobjtype(const LoxValue value, LoxObjectType type) {
    return is_loxobject(value) && as_loxobject(value)->type == type;
}

#define get_objtype(v)      (as_loxobject(v)->type)
#define is_loxstring(v)     (isobjtype(v, OBJ_STRING))

#define as_loxstring(v)     ((LoxString*)as_loxobject(v))
#define as_cstring(v)       (as_loxstring(v)->buffer)

#endif /* CLOX_OBJECT_H */
