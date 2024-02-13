#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

/* Start arrays off with 8 elements, or grow them by factors of 2. */
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

/** 
 * Think of this like a C++ templated function, but worse! 
 * Note that we wrap `oldcapacity` in parentheses so that if it's another macro
 * or expression in itself, we can correctly evaluate it first.
 */
#define GROW_ARRAY(T, pointer, oldcapacity, newcapacity) \
    reallocate(pointer, sizeof(T) * (oldcapacity), sizeof(T) * (newcapacity))

/* Indicates to `reallocate` we need to free `pointer`. */
#define FREE_ARRAY(T, pointer, oldcapacity) \
    reallocate(pointer, sizeof(T) * (oldcapacity), 0)

/**
 * @brief   Used for all dynamic memory management in `clox`. Please see the
 *          details below!
 *          
 * @details The values of `oldsize` and `newsize` can control particular
 *          behaviours of the function:
 * 
 *          | oldsize | newsize | actions
 *          |---------|---------|-----------------------------------------------
 *          |   0     |  n > 0  | Allocate new block.
 *          |  n > 0  |    0    | Free allocation.
 *          |  n > 0  | n < old | Shrink existing allocation.
 *          |  n > 0  | n > old | Grow existing allocation.
 *          |---------|---------|-----------------------------------------------
 */
void *reallocate(void *pointer, size_t oldsize, size_t newsize);

#endif /* CLOX_MEMORY_H */
