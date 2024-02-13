#include <stdlib.h>

#include "memory.h"

void *reallocate(void *pointer, size_t oldsize, size_t newsize)
{
    if (newsize == 0) {
        free(pointer);
        return NULL;
    }
    // When `oldsize == 0`, equivalent to malloc'ing a new pointer.
    // Otherwise, move `pointer` to a new block. If success, `pointer` is freed.
    void *result = realloc(pointer, newsize);
    if (result == NULL) {
        fprintf(stderr, "Failed to (re)allocate memory!");
        exit(1); // Nothing we can do about this so just get outta here
    }
    return result;
}
