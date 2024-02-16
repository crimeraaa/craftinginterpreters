#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

/* III:18.1: Tagged Unions */
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER
} LoxValueType;

/* Note that in arrays, the compiler will likely add padding anyway! */
typedef struct {
    LoxValueType type; // Tag to keep track of what datatype we're at.
    union {
        bool boolean;
        double number;
    } as; // Holds the actual in-memory value for use.
} LoxValue;

/* Test a `LoxValue`'s type tag. */
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)

/* Interpret a `LoxValue`'s underlying value. */
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)

/* Create a new `LoxValue` of (or reset an existing one to) the given type. */
#define BOOL_VAL(value)     ((LoxValue){VAL_BOOL,   {.boolean = value}})
#define NIL_VAL             ((LoxValue){VAL_NIL,    {.number = 0}})
#define NUMBER_VAL(value)   ((LoxValue){VAL_NUMBER, {.number = value}})

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
