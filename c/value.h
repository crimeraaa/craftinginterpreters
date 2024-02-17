#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

/** 
 * III:19.1: Values and Objects 
 *
 * Forward declared to avoid circular dependencies.
 */

typedef struct LoxObject LoxObject;
typedef struct LoxString LoxString;

/* III:18.1: Tagged Unions */
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJECT, // III:19.1: Values and Objects
} LoxValueType;

/* Note that in arrays, the compiler will likely add padding anyway! */
typedef struct {
    LoxValueType type; // Tag to keep track of what datatype we're at.
    union {
        bool boolean;
        double number;
        LoxObject *object;
    } as; // Holds the actual in-memory value for use.
} LoxValue;

/* Test a `LoxValue`'s type tag. */
#define is_loxbool(v)      ((v).type == VAL_BOOL)
#define is_loxnil(v)       ((v).type == VAL_NIL)
#define is_loxnumber(v)    ((v).type == VAL_NUMBER)
#define is_loxobject(v)    ((v).type == VAL_OBJECT)

/* Interpret a `LoxValue`'s underlying value. */
#define as_loxbool(v)      ((v).as.boolean)
#define as_loxnumber(v)    ((v).as.number)
#define as_loxobject(v)    ((v).as.object)

/* Create a new `LoxValue` using the value of the given type. */
#define make_loxbool(v)    ((LoxValue){VAL_BOOL,   {.boolean = v}})
#define make_loxnil        ((LoxValue){VAL_NIL,    {.number = 0}})
#define make_loxnumber(v)  ((LoxValue){VAL_NUMBER, {.number = v}})
#define make_loxobject(v)  ((LoxValue){VAL_OBJECT, {.object = (LoxObject*)v}})

/* Dynamic array which can act as a memory pool for values. */
typedef struct {
    int count;
    int capacity;
    LoxValue *values;
} LoxValueArray; 

void init_valuearray(LoxValueArray *array);
void write_valuearray(LoxValueArray *array, LoxValue value);
void free_valuearray(LoxValueArray *array);
void print_value(LoxValue value);

/**
 * Compare 2 `LoxValue`s. If they're different types, returns false.
 * If they're both `nil`, returns true. 
 * Otherwise, compare them based on their type (e.g. `number`).
 * 
 * Note that we CANNOT just use `memcmp` safely due to padding and unused memory!
 */
bool values_equal(LoxValue lhs, LoxValue rhs);

#endif /* CLOX_VALUE_H */
