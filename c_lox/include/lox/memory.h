#ifndef _CLOX_MEMORY_H
#define _CLOX_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(cap) \
  (cap < 8? 8 : cap * 2)

#define GROW_ARRAY(type, begptr, oldcap, cap) \
  (type*)reallocate(begptr, sizeof(type) * oldcap, sizeof(type) * cap)

#define FREE_ARRAY(type, begptr, cap) \
  reallocate(begptr, sizeof(type) * cap, 0)

void *reallocate(void *ptr, size_t osize, size_t nsize) {
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  void *result = realloc(ptr, nsize);
  if (result == NULL)
    exit(EXIT_FAILURE);
  return result;
}

#endif //_CLOX_MEMORY_H