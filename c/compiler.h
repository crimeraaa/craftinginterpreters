#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"

/**
 * @brief   Takes the user's source code (either a REPL line or a script) and
 *          fill the LoxChunk `chunk` with the appropriate bytecode.
 *          
 * @param   source      Line from REPL or an entire script's code.
 * @param   chunk       Some initialized but empty LoxChunk.
 * 
 * @return  True if everything went well, else false on some compiler error.
 */
bool compile(const char *source, LoxChunk *chunk);

#endif /* CLOX_COMPILER_H */
