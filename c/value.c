#include <stdio.h>

#include "memory.h"
#include "object.h"
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
        array->capacity = grow_capacity(oldcapacity);
        array->values = grow_array(LoxValue, 
            array->values, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void free_valuearray(LoxValueArray *array)
{
    free_array(LoxValue, array->values, array->capacity);
    init_valuearray(array);
}

void print_value(LoxValue value)
{
    switch (value.type) {
    case VAL_BOOL: printf(as_loxbool(value) ? "true" : "false"); break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", as_loxnumber(value)); break;
    case VAL_OBJECT: print_object(value); break;
    }
}

bool values_equal(LoxValue lhs, LoxValue rhs)
{
    if (lhs.type != rhs.type) {
        return false;
    }
    switch (lhs.type) {
    case VAL_BOOL: return lhs.as.boolean == rhs.as.boolean;
    case VAL_NIL: return true;
    case VAL_NUMBER: return lhs.as.number == rhs.as.number;
    case VAL_OBJECT: {
        LoxString *s1 = as_loxstring(lhs);
        LoxString *s2 = as_loxstring(rhs);
        if (s1->length == s2->length) {
            return memcmp(s1->buffer, s2->buffer, s1->length);
        }
        return false;
    }
    default: return false; // Should be unreachable.
    }
}
