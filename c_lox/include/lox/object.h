#ifndef _CLOX_OBJECT_H
#define _CLOX_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)

#define IS_STRING(value)   is_object_type(value, OBJ_STRING)
#define IS_NATIVE(value)   is_object_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value)  is_object_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) is_object_type(value, OBJ_FUNCTION)

#define AS_NATIVE(value)   ((ObjectNative *)AS_OBJECT(value))->function
#define AS_STRING(value)   ((ObjectString *)AS_OBJECT(value))
#define AS_CSTRING(value)  (AS_STRING(value))->chars
#define AS_FUNCTION(value) ((ObjectFunction *)AS_OBJECT(value))
#define AS_CLOSURE(value)  ((ObjectClosure *)AS_OBJECT(value))
#define UNWRAP_CLOSURE(value) (AS_CLOSURE(value))->function

#define ALLOCATE_OBJECT(Type, ObjectType) \
  (Type *)allocate_object(sizeof(Type), ObjectType)

uint32_t hash_string(const char*, int);

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_CLOSURE,
  OBJ_UPVALUE
} ObjectType;

struct Object {
  ObjectType type;
  Object* next;
};

typedef struct {
  Object object;
  char* chars;
  uint32_t hash;
  int length;
} ObjectString;

typedef struct {
  Object object;
  int arity;
  int upvalue_count;
  Chunk chunk;
  ObjectString* name;
} ObjectFunction;

typedef Value(*NativeFn)(int, Value*);

typedef struct {
  Object object;
  NativeFn function;
} ObjectNative;

typedef struct {
  Object object;
  Value* location;
} ObjectUpvalue;

typedef struct {
  Object object;
  ObjectFunction* function;
  ObjectUpvalue** upvalues;
  int upvalue_count;
} ObjectClosure;

void new_object(Object*);
ObjectUpvalue* new_upvalue(Value*);
ObjectClosure* new_closure(ObjectFunction*);
ObjectFunction* new_function();
ObjectNative* new_native(NativeFn);
bool intern_string(ObjectString*);
ObjectString* table_find_istring(char*, int, uint32_t);

bool is_object_type(Value value, ObjectType type) {
  return IS_OBJECT(value) && OBJECT_TYPE(value) == type;
}

Object* allocate_object(size_t size, ObjectType type) {
  Object* object = (Object*)reallocate(NULL, 0, size);
  object->type = type;
  object->next = NULL;
  new_object(object);
  return object;
}

ObjectNative* new_native(NativeFn function) {
  ObjectNative* native = ALLOCATE_OBJECT(ObjectNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjectString* allocate_string(char* payload, int size, uint32_t hash) {
  ObjectString* string = table_find_istring(payload, size, hash);
  if ( string != NULL ) return string;
  string = ALLOCATE_OBJECT(ObjectString, OBJ_STRING);
  string->chars = payload;
  string->length = size;
  string->hash = hash;
  intern_string(string);
  return string;
}

ObjectString* copy_string(const char* chars, int size) {
  uint32_t hash = hash_string(chars, size);
  char* payload = ALLOCATE(char, size + 1);
  memcpy(payload, chars, size);
  payload[size] = '\0';
  return allocate_string(payload, size, hash);
}

void value_function_print(ObjectFunction* function) {
  if ( function->name == NULL ) printf("<script>");
  else printf("<fn %s>", function->name->chars);
}

void value_oprint(Value value) {
  switch ( OBJECT_TYPE(value) ) {
  case OBJ_UPVALUE: puts("upvalue");                             break;
  case OBJ_NATIVE: printf("<native fn>");                        break;
  case OBJ_STRING: printf("%s", AS_CSTRING(value));              break;
  case OBJ_FUNCTION: value_function_print(AS_FUNCTION(value));   break;
  case OBJ_CLOSURE: value_function_print(UNWRAP_CLOSURE(value)); break;
  default: printf("Unknown object: %d", OBJECT_TYPE(value));     break;
  }
}

void object_delete(Object* object) {
#ifdef CLOX_ODEL_TRACE
  printf("DEL_OBJECT: ");
  value_oprint(OBJECT_VAL(object));
  putchar(10);
#endif
  switch ( object->type ) {
  case OBJ_STRING: {
    ObjectString* string = (ObjectString*)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjectString, object);                   break;
  }
  case OBJ_FUNCTION:
    chunk_delete(&((ObjectFunction*)object)->chunk);
    FREE(ObjectFunction, object);                 break;
  case OBJ_NATIVE:
    FREE(ObjectNative, object);                   break;
  case OBJ_CLOSURE:
    FREE_ARRAY(ObjectUpvalue*,
      ((ObjectClosure*)object)->upvalues,
      ((ObjectClosure*)object)->upvalue_count
    );
    FREE(ObjectClosure, object);                  break;
  case OBJ_UPVALUE:
    FREE(ObjectUpvalue, object);                  break;
  }
}

void objects_delete(Object* objects) {
  Object* object;
  while ( objects != NULL ) {
    object = objects;
    objects = objects->next;
    object_delete(object);
  }
}

#endif //_CLOX_OBJECT_H
