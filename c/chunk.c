#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void chunk_init(LoxChunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    valuearray_init(&chunk->constants); // '&' op has very low precedence
}

void chunk_free(LoxChunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    valuearray_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_write(LoxChunk *chunk, uint8_t opcode, int line)
{
    // Try to resize buffer.
    if (chunk->count + 1 > chunk->capacity) {
        int oldcapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldcapacity);

        chunk->code = GROW_ARRAY(uint8_t, 
            chunk->code, oldcapacity, chunk->capacity);

        chunk->lines = GROW_ARRAY(int, 
            chunk->lines, oldcapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = opcode;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int chunk_add_constant(LoxChunk *chunk, LoxValue value)
{
    valuearray_write(&chunk->constants, value);
    return chunk->constants.count - 1; // Index where the constant was appended
}
