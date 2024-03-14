#ifndef _CLOX_VALUE_H
#define _CLOX_VALUE_H

#include <stdio.h>
#include "memory.h"
#include "common.h"

CLOX_BEG_DECLS

typedef enum {
  VAL_NIL,
  VAL_BOOL,
  VAL_NUMBER,
  VAL_OBJECT,
} ValueType;

typedef struct Object Object;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Object* object;
  } payload;
} Value;


#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJECT(value) ((value).type == VAL_OBJECT)

#define AS_BOOL(value) ((value).payload.boolean)
#define AS_NUMBER(value) ((value).payload.number)
#define AS_OBJECT(value) ((value).payload.object)

#define NIL_VAL ((Value){ .type=VAL_NIL, .payload={ .number = 0 } })
#define BOOL_VAL(value) ((Value){ .type=VAL_BOOL, .payload={ .boolean = value } })
#define NUMBER_VAL(value) ((Value){ .type=VAL_NUMBER, .payload={ .number = value } })
#define OBJECT_VAL(value) ((Value){ .type=VAL_OBJECT, .payload={ .object = (Object *)(value) } })


typedef struct {
  Value* values;
  int capacity;
  int count;
} ValueArray;

void value_init(ValueArray* array) {
  *array = (ValueArray){ NULL, 0, 0 };
}

void value_delete(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
}

void value_append(ValueArray* array, Value value) {
  if ( array->capacity < array->count + 1 ) {
    int capacity = array->capacity;
    array->capacity = GROW_CAPACITY(capacity);
    array->values = GROW_ARRAY(Value, array->values, capacity, array->capacity);
  }
  array->values[array->count] = value;
  array->count++;
}

void value_oprint(Value object);

void value_print(Value value) {
  switch ( value.type ) {
  case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false");  break;
  case VAL_NUMBER: printf("%g", AS_NUMBER(value));             break;
  case VAL_OBJECT: value_oprint(value);                        break;
  case VAL_NIL: printf("nil");                                 break;
  }
}

CLOX_END_DECLS

#endif //_CLOX_VALUE_H
