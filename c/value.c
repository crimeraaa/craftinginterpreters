#include <stdio.h>

#include "memory.h"
#include "value.h"

void init_valuearray(LoxValueArray *array)
{
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void write_valuearray(LoxValueArray *array, LoxValue value)
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

void free_valuearray(LoxValueArray *array)
{
    FREE_ARRAY(LoxValue, array->values, array->capacity);
    init_valuearray(array);
}

void print_value(LoxValue value)
{
    switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    }
}

bool values_equal(LoxValue lhs, LoxValue rhs)
{
    if (lhs.type != rhs.type) {
        return false;
    }
    switch (lhs.type) {
    case VAL_BOOL: return AS_BOOL(lhs) == AS_BOOL(rhs);
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(lhs) == AS_NUMBER(rhs);
    default: return false; // Should be unreachable.
    }
}
