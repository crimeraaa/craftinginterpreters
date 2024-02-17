#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void init_chunk(LoxChunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_valuearray(&chunk->constants); // '&' op has very low precedence
}

void free_chunk(LoxChunk *chunk)
{
    free_array(uint8_t, chunk->code, chunk->capacity);
    free_array(int, chunk->lines, chunk->capacity);
    free_valuearray(&chunk->constants);
    init_chunk(chunk);
}

void write_chunk(LoxChunk *chunk, uint8_t byte, int line)
{
    // Try to resize buffer.
    if (chunk->count + 1 > chunk->capacity) {
        int oldcapacity = chunk->capacity;
        chunk->capacity = grow_capacity(oldcapacity);

        chunk->code = grow_array(uint8_t, 
            chunk->code, oldcapacity, chunk->capacity);

        chunk->lines = grow_array(int, 
            chunk->lines, oldcapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int add_constant(LoxChunk *chunk, LoxValue value)
{
    write_valuearray(&chunk->constants, value);
    return chunk->constants.count - 1; // Index where the constant was appended
}
