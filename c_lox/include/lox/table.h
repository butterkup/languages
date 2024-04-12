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

void table_print(Table*);
void entry_print(Entry*);

void table_init(Table* table) {
  table->entries = NULL;
  table->capacity = -1;
  table->count = 0;
}

void table_delete(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity + 1);
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

/* cap must be a mask: capacity - 1 */
Entry* entry_find(Entry* const entries, int cap, ObjectString* const key) {
  // uint64_t idx = key->hash % cap;
  uint64_t idx = key->hash & cap;
  Entry* tombstone = NULL, * entry = entries + idx;
  for ( ;; idx = (idx + 1) & cap, entry = entries + idx )
    if ( entry->key == key ) return entry;
    else if ( entry->key == NULL )
      if ( IS_NIL(entry->value) )
        return tombstone ? tombstone : entry;
      else if ( tombstone == NULL ) tombstone = entry;
  return NULL; // UNREACHABLE
}

ObjectString* table_find_string(Table* table, const char* chars, int length, uint64_t hash) {
  if ( table->count == 0 ) return NULL;
  uint64_t index = hash & table->capacity;
  Entry* entry = table->entries + index;
  for ( ;; index = (index + 1) & table->capacity,
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
  Entry* new_entries = ALLOCATE(Entry, new_capacity + 1);
  Entry* entry, * slot;
  int count = 0;
  // Set all entries to NULL and NIL_VAL
  for ( int idx = 0; idx <= new_capacity; idx++ )
    entry_init(new_entries + idx);
  // Transfer the entries from old loc to new loc
  // by mapping slots appropriately
  for ( int idx = 0; idx <= table->capacity; idx++ ) {
    entry = table->entries + idx;
    if ( !entry->key ) continue;
    slot = entry_find(new_entries, new_capacity, entry->key);
    slot->key = entry->key;
    slot->value = entry->value;
    count++;
  }
  // Free the old entries correctly.
  // table_delete(table);
  FREE_ARRAY(Entry, table->entries, table->capacity + 1);
  // Set the new adjusted table values
  table->capacity = new_capacity;
  table->entries = new_entries;
  table->count = count;
}

bool table_set(Table* table, ObjectString* key, Value value) {
  if ( table->count + 1 > (table->capacity + 1) * TABLE_MAX_LOAD )
    table_adjust_cap(table, GROW_CAPACITY(table->capacity + 1) - 1);
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
  for ( ; cap >= 0; --cap, ++e ) if ( e->key )
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
  // entry->key = NULL; entry->value = BOOL_VAL(true);
  *entry = (Entry){ NULL, BOOL_VAL(true) };
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
  for ( ; c >= 0; --c, ++e) if ( e->key )
    entry_print(e), printf(", ");
  putchar('}');
}

#endif //_CLOX_TABLE_H
