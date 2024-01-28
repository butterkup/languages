#ifndef _CLOX_VALUE_H
#define _CLOX_VALUE_H

#include <stdio.h>
#include "memory.h"
#include "common.h"

CLOX_BEG_DECLS

typedef double Value;

typedef struct {
  Value *values;
  int capacity;
  int count;
} ValueArray;

void value_init(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void value_delete(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  value_init(array);
}

void value_append(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int capacity = array->capacity;
    array->capacity = GROW_CAPACITY(capacity);
    array->values = GROW_ARRAY(Value, array->values, capacity, array->capacity);
  }
  array->values[array->count] = value;
  array->count++;
}

void value_print(Value value) {
  printf("%g", value);
}

CLOX_END_DECLS

#endif //_CLOX_VALUE_H
