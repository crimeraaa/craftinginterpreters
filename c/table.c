#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

/** 
 * Use this to manage individual hashtables' load factor, 
 * Remember, the higher the load factor, the more collisions are likely!
 * 
 * This constant allows us to only grow the table when it's 75% full, as we want
 * to preallocate a bunch of memory to avoid constantly reallocating.
 */
#define TABLE_MAX_LOAD 0.75

void init_table(LoxTable *self) {
    self->count = 0;
    self->capacity = 0;
    self->entries = NULL; 
}

void free_table(LoxTable *self) {
    free_array(LoxEntry, self->entries, self->capacity);
    init_table(self);
}

/**
 * Given a key, figure out which index in the array we can insert to.
 * We use linear probing and collision handling for our hashtable.
 * 
 * We try to return a pointer to said index for you to write into.
 * 
 * For collisions, this assumes that there IS at least 1 empty index in the array.
 * Otherwise, we'd end up in an infinite loop! So be sure to check beforehand!
 */
static LoxEntry *find_entry(LoxEntry *entries, int capacity, LoxString *key) {
    uint32_t index = key->hash % capacity;
    LoxEntry *tombstone = NULL;
    for (;;) {
        LoxEntry *entry = &entries[index];
        if (entry->key == NULL) {
            if (is_loxnil(entry->value)) {
                // Fully empty entry so we need to finish up the function.
                return (tombstone != NULL) ? tombstone : entry;
            } else {
                // If we haven't set our tombstone, set it now for later use.
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            // If NULL, the array at this index is empty so we can write to it!
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

bool table_get(LoxTable *self, LoxString *key, LoxValue *out) {
    // If count 0 zero then entries is NULL so don't even dare to dereference it!
    if (self->count == 0) {
        return false;
    }
    LoxEntry *entry = find_entry(self->entries, self->capacity, key);
    if (entry->key == NULL) {
        return false;
    }
    *out = entry->value;
    return true;
}

/**
 * Adjust the given hashtable with the new capacity, appropriately recalculating
 * the collisions and such.
 */
static void adjust_capacity(LoxTable *self, int capacity) {
    LoxEntry *entries = allocate(LoxEntry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = make_loxnil;
    }
    // Rebuild the table from scratch as hashcodes likely changed.
    //
    // We reset the count so we can only increment it as we rebuild using
    // non-tombstone values.
    self->count = 0;
    for (int i = 0; i < self->capacity; i++) {
        LoxEntry *src = &self->entries[i];
        if (src->key == NULL) {
            continue;
        }
        // Remember to pass the NEW entires array, not the OLD one!
        LoxEntry *dst = find_entry(entries, capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        self->count++; // Only increment when it's a non-tombstone entry.
    }
    // Since we're about to change self->entries, let go of this memory.
    free_array(LoxEntry, self->entries, self->capacity);
    self->entries = entries;
    self->capacity = capacity;
}

bool table_set(LoxTable *self, LoxString *key, LoxValue value) {
    if (self->count + 1 > self->capacity * TABLE_MAX_LOAD) {
        int capacity = grow_capacity(self->capacity);
        adjust_capacity(self, capacity);
    }
    LoxEntry *entry = find_entry(self->entries, self->capacity, key);
    bool isnewkey = (entry->key == NULL);
    // Tombstones were previously valid elements that maybed added to the count,
    // so we don't consider them as new keys.
    if (isnewkey && is_loxnil(entry->value)) {
        self->count++;
    }
    entry->key = key;
    entry->value = value;
    return isnewkey;
}

bool table_delete(LoxTable *self, LoxString *key) {
    // If this is zero then it's likely self->entries is NULL, so don't!
    if (self->count == 0) {
        return false;
    }
    // Get a pointer to the entry to be deleted from the hashtable.
    LoxEntry *entry = find_entry(self->entries, self->capacity, key);
    if (entry->key == NULL) {
        return false;
    }
    // Place a tombstone in this spot in the hashtable.
    // Don't use nil since NULL and nil are considered an empty value, so we use
    // the Lox boolean true to indicate this is a tombstone.
    // Any non-nil Lox value would work.
    entry->key = NULL;
    entry->value = make_loxbool(true);
    return true;
}

void table_add_all(LoxTable *dst, LoxTable *src) {
    for (int i = 0; i < src->capacity; i++) {
        LoxEntry *entry = &src->entries[i];
        if (entry->key == NULL) {
            table_set(dst, entry->key, entry->value);
        }
    }
}

LoxString *table_findstring(LoxTable *self, const char *chars, int length, uint32_t hash) {
    if (self->count == 0) {
        return NULL;
    }
    uint32_t index = hash % self->capacity;
    // Looks similar to find_entry() but we don't have an ObjString to work with.
    for (;;) {
        LoxEntry *entry = &self->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty and non-tombstone entry.
            if (is_loxnil(entry->value)) {
                return NULL;
            }
        } else if (entry->key->length == length && entry->key->hash == hash) {
            // Same length and same hash, now see if they're the same string!
            if (memcmp(entry->key->buffer, chars, length) == 0) {
                return entry->key; // Found it!
            }
        }
        index = (index + 1) % self->capacity;
    }
}
