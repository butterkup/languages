#ifndef _CLOX_VALUE_H
#define _CLOX_VALUE_H

#include <stdio.h>
#include "memory.h"
#include "common.h"

CLOX_BEG_DECLS

typedef struct Object Object;

#ifdef NAN_BOXING_OPT

typedef  uint64_t Value;

// WARNING: Do not change these constants
// The value macros are calculated with them
# define _QNAN      0x7f'fc'00'00'00'00'00'00 // 2nd to 13th are on
# define _SIGN_BIT  0x80'00'00'00'00'00'00'00 // 1 << 63   1st bit is on
# define _ERROR_BIT 0x00'04'00'00'00'00'00'00 // 1 << 50   14th bit is on
# define _TAG_NIL   1
# define _TAG_FALSE 2
# define _TAG_TRUE  3
# define _OBJECT_BITS (_SIGN_BIT | _QNAN)
# define _ERROR_BITS  (_OBJECT_BITS | _ERROR_BIT)

Value _double_to_value(double num) { return *((Value*)&num); }
double _value_to_double(Value val) { return *((double*)&val); }

// Lox NaN-Tagging Value macros
# define NUMBER_VAL(num) _double_to_value(num)
# define AS_NUMBER(val) _value_to_double(val)
# define IS_NUMBER(val) (((val) & _QNAN) != _QNAN)

# define NIL_VAL ((Value)(_QNAN | _TAG_NIL))
# define IS_NIL(val) ((val) == NIL_VAL)

# define FALSE_VAL ((Value)(_QNAN | _TAG_FALSE))
# define TRUE_VAL ((Value)(_QNAN | _TAG_TRUE))
# define BOOL_VAL(val) ((val)? TRUE_VAL: FALSE_VAL)
# define AS_BOOL(val) ((val) == TRUE_VAL)
# define IS_BOOL(val) (((val) | _TAG_TRUE) == TRUE_VAL)

# define OBJECT_VAL(ptr) ((Value)(_OBJECT_BITS | (uint64_t)(ptr)))
# define AS_OBJECT(val) ((Object*)((val) & ~_OBJECT_BITS))
# define IS_OBJECT(val) (((val) & _OBJECT_BITS) == _OBJECT_BITS)

# define ERROR_VAL(ptr) ((Value)(_ERROR_BITS | (uint64_t)(ptr)))
# define AS_ERROR(val) ((const char*)((val) & ~_ERROR_BITS))
# define IS_ERROR(val) (((val) & _ERROR_BITS) == _ERROR_BITS)

#else

typedef enum {
  VAL_NIL,
  VAL_BOOL,
  VAL_ERROR,
  VAL_NUMBER,
  VAL_OBJECT,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Object* object;
  } payload;
} Value;

// Lox Struct Value macros
# define NUMBER_VAL(value) ((Value){ .type=VAL_NUMBER, .payload.number = value })
# define AS_NUMBER(value) ((value).payload.number)
# define IS_NUMBER(value) ((value).type == VAL_NUMBER)

# define NIL_VAL ((Value){ .type=VAL_NIL, .payload.number = 0 })
# define IS_NIL(value) ((value).type == VAL_NIL)

# define FALSE_VAL BOOL_VAL(false)
# define TRUE_VAL BOOL_VAL(true)
# define BOOL_VAL(value) ((Value){ .type=VAL_BOOL, .payload.boolean = value })
# define AS_BOOL(value) ((value).payload.boolean)
# define IS_BOOL(value) ((value).type == VAL_BOOL)

# define OBJECT_VAL(value) ((Value){ .type=VAL_OBJECT, .payload.object = (Object *)(value) })
# define AS_OBJECT(value) ((value).payload.object)
# define IS_OBJECT(value) ((value).type == VAL_OBJECT)

# define ERROR_VAL(value)  ((Value){ .type=VAL_ERROR, .payload.object = (Object*)(value) })
# define AS_ERROR(value)  ((const char*)(value).payload.object)
# define IS_ERROR(value)  ((value).type == VAL_ERROR)

#endif

Value stack_pop();
void stack_push(Value);

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
  stack_push(value);
  if ( array->capacity < array->count + 1 ) {
    int capacity = array->capacity;
    array->capacity = GROW_CAPACITY(capacity);
    array->values = GROW_ARRAY(Value, array->values, capacity, array->capacity);
  }
  stack_pop();
  array->values[array->count++] = value;
}

void value_oprint(Value object);

void value_print(Value value) {
#ifdef NAN_BOXING_OPT
  if ( IS_NUMBER(value) ) printf("%g", AS_NUMBER(value));
  else if ( IS_BOOL(value) ) printf(AS_BOOL(value) ? "true" : "false");
  else if ( IS_NIL(value) ) printf("nil");
  else if ( IS_OBJECT(value) ) value_oprint(value);
  else if ( IS_ERROR(value) ) printf("(Error: '%s')", AS_ERROR(value));
  else printf("Unknown Value: %ld", value);
#else
  switch ( value.type ) {
  case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false");  break;
  case VAL_NUMBER: printf("%g", AS_NUMBER(value));             break;
  case VAL_OBJECT: value_oprint(value);                        break;
  case VAL_NIL: printf("nil");                                 break;
  default: printf("Unknown Value: %d", value.type);            break;
  }
#endif
}

CLOX_END_DECLS

#endif //_CLOX_VALUE_H
