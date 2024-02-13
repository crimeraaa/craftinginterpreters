#include <stdio.h>

#include "memory.h"
#include "value.h"

void lox_init_valuearray(LoxValueArray *array)
{
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void lox_write_valuearray(LoxValueArray *array, LoxValue value)
{
    if (array->count + 1 > array->capacity) {
        int oldcapacity = array->capacity;
        array->capacity = LOX_GROW_CAPACITY(oldcapacity);
        array->values = LOX_GROW_ARRAY(LoxValue, 
            array->values, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void lox_free_valuearray(LoxValueArray *array)
{
    LOX_FREE_ARRAY(LoxValue, array->values, array->capacity);
    lox_init_valuearray(array);
}

void lox_print_value(LoxValue value)
{
    printf("%g", value);
}
