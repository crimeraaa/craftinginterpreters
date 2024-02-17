#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

void *reallocate(void *pointer, size_t oldsize, size_t newsize) {
    (void)oldsize;
    if (newsize == 0) {
        free(pointer);
        return NULL;
    }
    // When `oldsize == 0`, equivalent to malloc'ing a new pointer.
    // Otherwise, move `pointer` to a new block. If success, `pointer` is freed.
    void *result = realloc(pointer, newsize);
    if (result == NULL) {
        lox_dputs("reallocate", "Failed to (re)allocate memory!");
        exit(1); // Nothing we can do about this so just get outta here
    }
    return result;
}

static void free_object(LoxObject *object) {
    switch (object->type) {
    case OBJ_STRING: {
        LoxString *s = (LoxString*)object;
        free_array(char, s->buffer, s->length + 1);
        free_value(LoxString, object);
        break;
    }}
}

void free_objects(void) {
    LoxObject *object = vm.objects;
    while (object != NULL) {
        LoxObject *next = object->next;
        free_object(object);
        object = next;
    }
}
