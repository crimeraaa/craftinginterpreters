#ifndef CLOX_HASHTABLE_H
#define CLOX_HASHTABLE_H

#include "common.h"
#include "value.h"

/* We assume that hashtable keys are ALWAYS strings. */
typedef struct {
    LoxString *key;
    LoxValue value;
} LoxEntry;

/* A dynamic array of LoxEntry with a unique way of inserting elements. */
typedef struct {
    int count;
    int capacity;
    LoxEntry *entries;
} LoxTable;

/* Set the members to 0 and NULL. */
void init_table(LoxTable *self);

/* Walk through all allocations in the table and free them however. */
void free_table(LoxTable *self);

/**
 * If the given key exists, return true and set the out parameter.
 * Else return false and don't set the outparameter.
 */
bool table_get(LoxTable *self, LoxString *key, LoxValue *out);

/**
 * Adds the given key-value pair to the given hashtable.
 *
 * We don't reduce the count when a previous entry was deleted.
 * Remember that tombstones are a combination of NULL and Lox boolean true,
 * whereas truly empty entries are NULL and Lox nil.
 *
 * Now our hashtable's count refers to both valid entries and tombstones.
 * This is the lesser evil compared to not counting them.
 */
bool table_set(LoxTable *self, LoxString *key, LoxValue value);

/* Delete the entry found at `key` and create the appropriate tombstones. */
bool table_delete(LoxTable *self, LoxString *key);

/**
 * Copy all entries from `src` to `dst`.
 * Note that Bob's original implementation is actually: (from, to)
 * meaning my order is the other way around.
 *
 * Whenever a non-empty array index in `src` is found, we copy it to `dst`.
 */
void table_add_all(LoxTable *dst, LoxTable *src);

/**
 * Try to see if `chars` is an already interned string.
 *
 * If it is, returns the pointer to the LoxString we allocated for it.
 * This allows you to do pointer comparison between interned strings.
 *
 * Otherwise, if NULL, you do whatever you need to create/intern a new LoxString.
 */
LoxString *table_findstring(LoxTable *self, const char *chars, int length, uint32_t hash);

#endif /* CLOX_HASHTABLE_H */
