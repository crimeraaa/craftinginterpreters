#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static LoxObject *allocate_object(size_t size, LoxObjectType type) {
    LoxObject *object = reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects; // Classic linked list switcharoo
    vm.objects = object;
    return object;
}

/* Do need the cast because C doesn't do implicit pointer conversions. */
#define allocate_object(T, tag)     (T*)allocate_object(sizeof(T), tag)

static LoxString *allocate_string(char *buffer, int length) {
    LoxString *s = allocate_object(LoxString, OBJ_STRING);
    s->length = length;
    s->buffer = buffer;
    return s;
}

LoxString *copy_string(const char *literal, int length) {
    char *buffer = allocate(char, length + 1);
    memcpy(buffer, literal, length);
    buffer[length] = '\0';
    return allocate_string(buffer, length);
}

LoxString *take_string(char *buffer, int length) {
    return allocate_string(buffer, length);
}

void print_object(LoxValue value) {
    switch(get_objtype(value)) {
    case OBJ_STRING:
        printf("%s", as_cstring(value));
        break;
    }
}
