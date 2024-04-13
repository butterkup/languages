#ifndef _CLOX_TABLE_H
#define _CLOX_TABLE_H

#include "common.h"
#include "value.h"
#include "memory.h"

#define TABLE_MAX_LOAD 0.75f

typedef struct {
  ObjectString* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

#include "object.h"

#ifdef TABLE_AND_FOLD_OPT
# define TAB_COMP_OP <=
# define TAB_FOLD_OP &
#else
# define TAB_COMP_OP <
# define TAB_FOLD_OP %
#endif

void table_print(Table*);
void entry_print(Entry*);

void table_init(Table* table) {
  table->entries = NULL;
#ifdef TABLE_AND_FOLD_OPT
  table->capacity = -1;
#else
  table->capacity = 0;
#endif
  table->count = 0;
}

void table_delete(Table* table) {
  FREE_ARRAY(Entry, table->entries,
#ifdef TABLE_AND_FOLD_OPT
    table->capacity + 1
#else
    table->capacity
#endif
  );
}

uint64_t fnv1a_algo(const char* chars, int length) {
  uint64_t hash = 2'166'136'261u;
  for ( int idx = 0; idx < length; ++idx )
    hash *= (hash ^ chars[idx]);
  return hash;
}

uint64_t my_hash(const char* chars, int length) {
  uint64_t hash = length;
  for ( int idx = 0; idx < length; ++idx )
    hash = (hash << 2) ^ chars[idx];
  return hash;
}

uint64_t hash_string(const char* chars, int length) {
#ifndef HASH_FUNCTION
# define HASH_FUNCTION my_hash
#endif
  return HASH_FUNCTION(chars, length);
}

Entry* entry_find(Entry* const entries, int cap, ObjectString* const key) {
  uint64_t idx = key->hash TAB_FOLD_OP cap;
  Entry* tombstone = NULL, * entry = entries + idx;
  for ( ;; idx = (idx + 1) TAB_FOLD_OP cap, entry = entries + idx )
    if ( entry->key == key ) return entry;
    else if ( entry->key == NULL )
      if ( IS_NIL(entry->value) )
        return tombstone ? tombstone : entry;
      else if ( tombstone == NULL ) tombstone = entry;
  return NULL; // UNREACHABLE
}

ObjectString* table_find_string(Table* table, const char* chars, int length, uint64_t hash) {
  if ( table->count == 0 ) return NULL;
  uint64_t index = hash TAB_FOLD_OP table->capacity;
  Entry* entry = table->entries + index;
  for ( ;; index = (index + 1) TAB_FOLD_OP table->capacity,
    entry = table->entries + index )
    if ( entry->key == NULL && IS_NIL(entry->value) ) return NULL;
    else if ( entry->key &&
      entry->key->length == length &&
      entry->key->hash == hash &&
      !memcmp(entry->key->chars, chars, length) )
      return entry->key;
  return NULL;
}

void entry_init(Entry* entry) {
  entry->key = NULL;
  entry->value = NIL_VAL;
}

/* new_capacity must be a mask: capacity - 1 */
void table_adjust_cap(Table* table, int new_capacity) {
  Entry* new_entries =
#ifdef TABLE_AND_FOLD_OPT
    ALLOCATE(Entry, new_capacity + 1);
#else
    ALLOCATE(Entry, new_capacity);
#endif
  Entry* entry, * slot;
  int count = 0;
  // Set all entries to NULL and NIL_VAL
  for ( int idx = 0; idx TAB_COMP_OP new_capacity; idx++ )
    entry_init(new_entries + idx);
  // Transfer the entries from old loc to new loc
  // by mapping slots appropriately
  for ( int idx = 0; idx TAB_COMP_OP table->capacity; idx++ ) {
    entry = table->entries + idx;
    if ( !entry->key ) continue;
    slot = entry_find(new_entries, new_capacity, entry->key);
    slot->key = entry->key;
    slot->value = entry->value;
    count++;
  }
  // Free the old entries correctly.
  table_delete(table);
  // Set the new adjusted table values
  table->capacity = new_capacity;
  table->entries = new_entries;
  table->count = count;
}

bool table_set(Table* table, ObjectString* key, Value value) {
#ifdef TABLE_AND_FOLD_OPT
  if ( table->count + 1 > (table->capacity + 1) * TABLE_MAX_LOAD )
    table_adjust_cap(table, GROW_CAPACITY(table->capacity + 1) - 1);
#else
  if ( table->count + 1 > table->capacity * TABLE_MAX_LOAD )
    table_adjust_cap(table, GROW_CAPACITY(table->capacity));
#endif
  Entry* entry = entry_find(table->entries, table->capacity, key);
  bool new_entry = entry->key == NULL;
  if ( new_entry && IS_NIL(entry->value) ) table->count++;
  // entry->key = key; entry->value = value;
  *entry = (Entry){ key, value };
  return new_entry;
}

void table_concat(Table* to, Table* from) {
  Entry* e = from->entries;
  int cap = from->capacity;
  for ( ; cap TAB_COMP_OP 0; --cap, ++e ) if ( e->key )
    table_set(to, e->key, e->value);
}

bool table_get(Table* table, ObjectString* key, Value* value) {
  if ( table->count == 0 ) return false;
  Entry* target = entry_find(table->entries, table->capacity, key);
  if ( !target->key ) return false;
  *value = target->value;
  return true;
}

bool table_del(Table* table, ObjectString* key) {
  if ( table->count == 0 ) return false;
  Entry* entry = entry_find(table->entries, table->capacity, key);
  if ( !entry->key ) return false;
  // entry->key = NULL; entry->value = TRUE_VAL;
  *entry = (Entry){ NULL, TRUE_VAL };
  return true;
}

void entry_print(Entry* entry) {
  value_oprint(OBJECT_VAL(entry->key));
  printf(": ");
  value_print(entry->value);
}

void table_print(Table* table) {
  Entry* e = table->entries;
  int c = table->capacity;
  putchar('{');
  for ( ; c TAB_COMP_OP 0; --c, ++e ) if ( e->key )
    entry_print(e), printf(", ");
  putchar('}');
}

#endif //_CLOX_TABLE_H
