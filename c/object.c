#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h" 

/**
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV_hash_parameters 
 */
#define FNV_PRIME32  0x01000193
#define FNV_OFFSET32 0x811c9dc5
#define FNV_PRIME64  0x00000100000001b3
#define FNV_OFFSET64 0xcbf29ce484222325

static LoxObject *allocate_object(size_t size, LoxObjectType type) {
    LoxObject *object = reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects; // Classic linked list switcharoo
    vm.objects = object;
    return object;
}

/* Do need the cast because C doesn't do implicit pointer conversions. */
#define allocate_object(T, tag)     (T*)allocate_object(sizeof(T), tag)

static LoxString *allocate_string(char *buffer, int length, uint32_t hash) {
    LoxString *s = allocate_object(LoxString, OBJ_STRING);
    s->length = length;
    s->buffer = buffer;
    s->hash = hash;
    // More like a hash set rather than hash table, we only care about keys.
    table_set(&vm.strings, s, make_loxnil);
    return s;
}

/**
 * Implements the FNV-1a hash function, using the appropriately sized constants.
 * For now we use the 32 bit version.
 * See: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
static uint32_t hash_string(const char *key, int length) {
    uint32_t hash = FNV_OFFSET32;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= FNV_PRIME32;
    }
    return hash;
}

LoxString *copy_string(const char *literal, int length) {
    uint32_t hash = hash_string(literal, length);
    LoxString *interned = table_findstring(&vm.strings, literal, length, hash);
    // No need to allocate a new LoxString for one that's been interned already.
    if (interned != NULL) {
        return interned;
    }
    char *buffer = allocate(char, length + 1);
    memcpy(buffer, literal, length);
    buffer[length] = '\0';
    return allocate_string(buffer, length, hash);
}

LoxString *take_string(char *buffer, int length) {
    uint32_t hash = hash_string(buffer, length);
    LoxString *interned = table_findstring(&vm.strings, buffer, length, hash);
    if (interned != NULL) {
        // We can get rid of this pointer since the string was already interned.
        free_array(char, buffer, length + 1); 
        return interned;
    }
    return allocate_string(buffer, length, hash);
}

void print_object(LoxValue value) {
    switch(get_objtype(value)) {
    case OBJ_STRING:
        printf("%s", as_cstring(value));
        break;
    }
}
