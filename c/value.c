#include <stdio.h>

#include "memory.h"
#include "value.h"

void valuearray_init(LoxValueArray *array)
{
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void valuearray_write(LoxValueArray *array, LoxValue value)
{
    if (array->count + 1 > array->capacity) {
        int oldcapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldcapacity);
        array->values = GROW_ARRAY(LoxValue, 
            array->values, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void valuearray_free(LoxValueArray *array)
{
    FREE_ARRAY(LoxValue, array->values, array->capacity);
    valuearray_init(array);
}

void value_print(LoxValue value)
{
    printf("%g", value);
}
