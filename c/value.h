#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

/* Stupid but let's get something going for now. */
typedef double LoxValue;

/* Dynamic array which can act as a memory pool for values. */
typedef struct {
    int count;
    int capacity;
    LoxValue *values;
} LoxValueArray; 

void lox_init_valuearray(LoxValueArray *array);
void lox_write_valuearray(LoxValueArray *array, LoxValue value);
void lox_free_valuearray(LoxValueArray *array);
void lox_print_value(LoxValue value);

#endif /* CLOX_VALUE_H */
