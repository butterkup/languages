#ifndef _CLOX_MEMORY_H
#define _CLOX_MEMORY_H

#include "common.h"

CLOX_BEG_DECLS

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8? 8 : (capacity) * 2)

#define GROW_ARRAY(type, begptr, oldcap, capacity) \
  (type*)reallocate(begptr, sizeof(type) * (oldcap), sizeof(type) * (capacity))

#define FREE_ARRAY(type, begptr, capacity) \
  reallocate(begptr, sizeof(type) * (capacity), 0)

#define FREE(type, ptr) \
  reallocate(ptr, sizeof(type), 0)

#define ALLOCATE(Type, Size) \
  (Type *)reallocate(NULL, 0, sizeof(Type) * (Size))

void collect_garbage();
void update_gc_state(size_t old_size, size_t new_size);

void* reallocate(void* ptr, size_t osize, size_t nsize) {
  update_gc_state(osize, nsize);
#ifdef CLOX_GC_STRESS
  if (nsize > osize) collect_garbage();
#endif // CLOX_GC_STRESS
  // puts("~ realloc: start");
  if ( nsize == 0 ) {
    free(ptr);
    // puts("~ realloc: free");
    return NULL;
  }
  void* result = realloc(ptr, nsize);
  if ( !result ) exit(80);
  // puts("~ realloc: alloc");
  return result;
}

CLOX_END_DECLS

#endif //_CLOX_MEMORY_H
