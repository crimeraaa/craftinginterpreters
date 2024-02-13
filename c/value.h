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

void valuearray_init(LoxValueArray *array);
void valuearray_write(LoxValueArray *array, LoxValue value);
void valuearray_free(LoxValueArray *array);
void value_print(LoxValue value);

#endif /* CLOX_VALUE_H */
