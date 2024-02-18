#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void init_valuearray(LoxValueArray *self)
{
    self->count = 0;
    self->capacity = 0;
    self->values = NULL;
}

void write_valuearray(LoxValueArray *self, LoxValue value)
{
    if (self->count + 1 > self->capacity) {
        int oldcapacity = self->capacity;
        self->capacity = grow_capacity(oldcapacity);
        self->values = grow_array(LoxValue, 
            self->values, oldcapacity, self->capacity);
    }
    self->values[self->count] = value;
    self->count++;
}

void free_valuearray(LoxValueArray *self)
{
    free_array(LoxValue, self->values, self->capacity);
    init_valuearray(self);
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
    // III:20.5: Since we intern strings, pointer comparisons are now ok!
    case VAL_OBJECT: return as_loxobject(lhs) == as_loxobject(rhs);
    default: return false; // Should be unreachable.
    }
}
