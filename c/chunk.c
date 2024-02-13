#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void lox_init_chunk(LoxChunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    lox_init_valuearray(&chunk->constants); // '&' op has very low precedence
}

void lox_free_chunk(LoxChunk *chunk)
{
    LOX_FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    LOX_FREE_ARRAY(int, chunk->lines, chunk->capacity);
    lox_free_valuearray(&chunk->constants);
    lox_init_chunk(chunk);
}

void lox_write_chunk(LoxChunk *chunk, uint8_t byte, int line)
{
    // Try to resize buffer.
    if (chunk->count + 1 > chunk->capacity) {
        int oldcapacity = chunk->capacity;
        chunk->capacity = LOX_GROW_CAPACITY(oldcapacity);

        chunk->code = LOX_GROW_ARRAY(uint8_t, 
            chunk->code, oldcapacity, chunk->capacity);

        chunk->lines = LOX_GROW_ARRAY(int, 
            chunk->lines, oldcapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int lox_add_constant(LoxChunk *chunk, LoxValue value)
{
    lox_write_valuearray(&chunk->constants, value);
    return chunk->constants.count - 1; // Index where the constant was appended
}
