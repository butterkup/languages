#ifndef _CLOX_MEMORY_H
#define _CLOX_MEMORY_H

#include "common.h"

CLOX_BEG_DECLS

#define GROW_CAPACITY(capacity) \
  (capacity < 8? 8 : capacity * 2)

#define GROW_ARRAY(type, begptr, oldcap, capacity) \
  (type*)reallocate(begptr, sizeof(type) * oldcap, sizeof(type) * capacity)

#define FREE_ARRAY(type, begptr, capacity) \
  reallocate(begptr, sizeof(type) * capacity, 0)

#define FREE(type, ptr) \
  reallocate(ptr, sizeof(type), 0)

#define ALLOCATE(Type, Size) \
  (Type *)reallocate(NULL, 0, sizeof(Type) * (Size))


void* reallocate(void* ptr, size_t osize, size_t nsize) {
  if ( nsize == 0 ) {
    free(ptr);
    return NULL;
  }
  void* result = realloc(ptr, nsize);
  if ( result == NULL ) exit(EXIT_FAILURE);
  return result;
}

CLOX_END_DECLS

#endif //_CLOX_MEMORY_H
