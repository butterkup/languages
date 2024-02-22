#ifndef _CLOX_OBJECT_H
#define _CLOX_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) is_object_type(value, OBJ_STRING)
#define AS_STRING(value) ((ObjectString *)AS_OBJECT(value))
#define AS_CSTRING(value) (((ObjectString *)AS_OBJECT(value))->chars)

#define ALLOCATE_OBJECT(Type, ObjectType) \
  (Type *)allocate_object(sizeof(Type), ObjectType)

typedef struct ObjectString ObjectString;

typedef enum {
  OBJ_STRING
} ObjectType;


struct Object {
  ObjectType type;
  Object *next;
};

struct ObjectString {
  Object object;
  char *chars;
  int length;
};

void new_object(Object *);

bool is_object_type(Value value, ObjectType type) {
  return IS_OBJECT(value) && OBJECT_TYPE(value) == type;
}

Object *allocate_object(size_t size, ObjectType type) {
  Object *object = (Object *)reallocate(NULL, 0, size);
  object->type = type;
  new_object(object);
  return object;
}

ObjectString *allocate_string(char *payload, int size) {
  ObjectString *string = ALLOCATE_OBJECT(ObjectString, OBJ_STRING);
  string->chars = payload;
  string->length = size;
  return string;
}

ObjectString *copy_string(const char *chars, int size) {
  char *payload = ALLOCATE(char, size + 1);
  memcpy(payload, chars, size);
  payload[size] = '\0';
  return allocate_string(payload, size);
}


void value_oprint(Value value) {
  switch (OBJECT_TYPE(value)) {
  case OBJ_STRING: printf("\"%s\"", AS_CSTRING(value));        break;
  default: printf("Unknown object: %d", OBJECT_TYPE(value)); break;
  }
}

void object_delete(Object *object) {
#ifdef CLOX_ODEL_TRACE
  printf("DEL_OBJECT: ");
  value_oprint(OBJECT_VAL(object));
  putchar(10);
#endif
  switch (object->type) {
    case OBJ_STRING: 
    ObjectString *string = (ObjectString *)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjectString, object);
    break;
  }
}

void objects_delete(Object *objects) {
  Object *object;
  while(objects != NULL) {
    object = objects;
    objects = objects->next;
    object_delete(object);
  }
}

#endif //_CLOX_OBJECT_H