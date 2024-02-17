#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

/**
 * III:19.3 
 * 
 * Helper macro to quickly allocate a new pointer to the given type of the
 * given amount.
 */
#define allocate(T, N)      reallocate(NULL, 0, sizeof(T) * (N))

#define free_value(T, Ptr)  reallocate(Ptr, sizeof(T), 0)

/** 
 * Start arrays off with 8 elements, or grow them by factors of 2. 
 */
#define grow_capacity(N)    ((N) < 8 ? 8 : (N) * 2)

/** 
 * Think of this like a C++ templated function, but worse! 
 * Note that we wrap `oldcapacity` in parentheses so that if it's another macro
 * or expression in itself, we can correctly evaluate it first.
 */
#define grow_array(T, Ptr, Old, New) \
    reallocate(Ptr, sizeof(T) * (Old), sizeof(T) * (New))

/** 
 * Indicates to `reallocate` we need to free `ptr`. 
 * 
 * At the moment, `Cap` doesn't really do anything.
 */
#define free_array(T, Ptr, Cap) reallocate(Ptr, sizeof(T) * (Cap), 0)

/**
 * @brief   Used for all dynamic memory management in `clox`. Please see the
 *          details below!
 *          
 * @details The values of `oldsize` and `newsize` can control particular
 *          behaviours of the function:
 * 
 *          | oldsize | newsize | actions
 *          |---------|---------|-----------------------------------------------
 *          |    0    |  n > 0  | Allocate new block.
 *          |  n > 0  |    0    | Free allocation.
 *          |  n > 0  | n < old | Shrink existing allocation.
 *          |  n > 0  | n > old | Grow existing allocation.
 *          |---------|---------|-----------------------------------------------
 *
 * @note    On catastrophic errors like a call to `realloc` returning `NULL`,
 *          we exit the program as it's likely you have worse problems.
 */
void *reallocate(void *pointer, size_t oldsize, size_t newsize);

void free_objects(void);

#endif /* CLOX_MEMORY_H */
