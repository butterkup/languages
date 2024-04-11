#ifndef _CLOX_OBJECT_H
#define _CLOX_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "memory.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)

#define IS_CLASS(value)        is_object_type(value, OBJ_CLASS)
#define IS_STRING(value)       is_object_type(value, OBJ_STRING)
#define IS_NATIVE(value)       is_object_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value)      is_object_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     is_object_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     is_object_type(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) is_object_type(value, OBJ_BOUND_METHOD)

#define AS_NATIVE_OBJ(value)   ((ObjectNative *)AS_OBJECT(value))
#define AS_NATIVE(value)       AS_NATIVE_OBJ(value)->function
#define AS_STRING(value)       ((ObjectString *)AS_OBJECT(value))
#define AS_CSTRING(value)      (AS_STRING(value))->chars
#define AS_FUNCTION(value)     ((ObjectFunction *)AS_OBJECT(value))
#define AS_CLOSURE(value)      ((ObjectClosure *)AS_OBJECT(value))
#define AS_CLASS(value)        ((ObjectClass *)AS_OBJECT(value))
#define AS_INSTANCE(value)     ((ObjectInstance *)AS_OBJECT(value))
#define AS_BOUND_METHOD(value) ((ObjectBoundMethod*)AS_OBJECT(value))
#define UNWRAP_CLOSURE(value)  (AS_CLOSURE(value))->function

#define ALLOCATE_OBJECT(Type, ObjectType) \
  (Type *)allocate_object(sizeof(Type), ObjectType)

uint64_t hash_string(const char*, int);

typedef enum {
  OBJ_BOUND_METHOD,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
  OBJ_STRING,
  OBJ_NATIVE,
  OBJ_CLASS
} ObjectType;

#define _STR(value) #value
#define CSOT(type) case OBJ_##type: return _STR(OBJECT_##type)

const char* strobjtype(ObjectType type) {
  switch ( type ) {
    CSOT(STRING);
    CSOT(FUNCTION);
    CSOT(NATIVE);
    CSOT(CLOSURE);
    CSOT(UPVALUE);
    CSOT(CLASS);
    CSOT(INSTANCE);
  default: "<UnknownObjectType>";
  }
}

#undef CSOT
#undef _STR

struct Object {
  ObjectType type;
  Object* next;
  bool is_marked;
};

typedef struct {
  Object object;
  char* chars;
  uint64_t hash;
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
  const char* name;
  NativeFn function;
} ObjectNative;

typedef struct ObjectUpvalue {
  Object object;
  Value* location;
  Value closed;
  struct ObjectUpvalue* next;
} ObjectUpvalue;

typedef struct {
  Object object;
  ObjectFunction* function;
  ObjectUpvalue** upvalues;
  int upvalue_count;
} ObjectClosure;

#include "table.h"

typedef struct {
  Object object;
  ObjectString* name;
  Table methods;
} ObjectClass;

typedef struct {
  Object object;
  ObjectClass* klass;
  Table fields;
} ObjectInstance;

typedef struct {
  Object object;
  Value receiver;
  ObjectClosure* method;
} ObjectBoundMethod;

void new_object(Object*);
ObjectInstance* new_instance(ObjectClass*);
ObjectClass* new_class(ObjectString*);
ObjectBoundMethod* new_bound_method(Value, ObjectClosure*);
ObjectUpvalue* new_upvalue(Value*);
ObjectClosure* new_closure(ObjectFunction*);
ObjectFunction* new_function();
ObjectNative* new_native(NativeFn, const char*);
void intern_string(ObjectString*);
ObjectString* table_find_istring(const char*, int, uint64_t);

bool is_object_type(Value value, ObjectType type) {
  return IS_OBJECT(value) && OBJECT_TYPE(value) == type;
}

Object* allocate_object(size_t size, ObjectType type) {
  Object* object = (Object*)reallocate(NULL, 0, size);
  object->is_marked = false;
  object->type = type;
  object->next = NULL;
  new_object(object);
#ifdef CLOX_GC_LOG
  printf("%p allocate %ld for %s\n", (void*)object, size, strobjtype(type));
#endif // CLOX_GC_LOG
  return object;
}

ObjectNative* new_native(NativeFn function, const char* name) {
  ObjectNative* native = ALLOCATE_OBJECT(ObjectNative, OBJ_NATIVE);
  native->function = function;
  native->name = name;
  return native;
}

ObjectClass* new_class(ObjectString* klass_name) {
  ObjectClass* klass = ALLOCATE_OBJECT(ObjectClass, OBJ_CLASS);
  klass->name = klass_name;
  table_init(&klass->methods);
  return klass;
}

ObjectBoundMethod* new_bound_method(Value receiver, ObjectClosure* method) {
  ObjectBoundMethod* bound_method = ALLOCATE_OBJECT(ObjectBoundMethod, OBJ_BOUND_METHOD);
  bound_method->receiver = receiver;
  bound_method->method = method;
  return bound_method;
}

ObjectInstance* new_instance(ObjectClass* klass) {
  ObjectInstance* instance = ALLOCATE_OBJECT(ObjectInstance, OBJ_INSTANCE);
  table_init(&instance->fields);
  instance->klass = klass;
  return instance;
}

ObjectString* allocate_string_noi(char* payload, int size, uint64_t hash) {
  ObjectString* string = ALLOCATE_OBJECT(ObjectString, OBJ_STRING);
  string->chars = payload;
  string->length = size;
  string->hash = hash;
  return string;
}

ObjectString* allocate_string(char* payload, int size, uint64_t hash) {
  ObjectString* string = table_find_istring(payload, size, hash);
  if ( string ) {
    FREE_ARRAY(char, payload, size);
    return string;
  }
  string = allocate_string_noi(payload, size, hash);
  intern_string(string);
  return string;
}

ObjectString* copy_string(const char* chars, int size) {
  uint64_t hash = hash_string(chars, size);
  char* payload = ALLOCATE(char, size + 1);
  memcpy(payload, chars, size);
  payload[size] = '\0';
  ObjectString* str = allocate_string(payload, size, hash);
  // printf("Copying[%p]: \"%.*s\"\n", str, size, chars);
  return str;
}

void value_function_print(ObjectFunction* function) {
  if ( function->name == NULL ) printf("<script>");
  else printf("<fn %s>", function->name->chars);
}

void print_bound_method(ObjectBoundMethod* bound_method) {
  printf("<bound ");
  value_print(OBJECT_VAL(bound_method->method));
  printf(" to ");
  value_print(bound_method->receiver);
  printf(">");
}

void value_oprint(Value value) {
  if ( !value.payload.object ) {
    printf("(NULL OBJECT)");
    return;
  }
#ifdef CLOX_OBJECT_TYPE
  printf("<%s at %p \"", strobjtype(OBJECT_TYPE(value)), value.payload.object);
#endif // CLOX_OBJECT_TYPE
  switch ( OBJECT_TYPE(value) ) {
  case OBJ_UPVALUE: printf("upvalue");                                                   break;
  case OBJ_STRING: printf("%s", AS_CSTRING(value));                                      break;
  case OBJ_FUNCTION: value_function_print(AS_FUNCTION(value));                           break;
  case OBJ_CLOSURE: value_function_print(UNWRAP_CLOSURE(value));                         break;
  case OBJ_CLASS: printf("<class %s>", AS_CLASS(value)->name->chars);                    break;
  case OBJ_BOUND_METHOD: print_bound_method(AS_BOUND_METHOD(value));                     break;
  case OBJ_NATIVE: printf("<native fn(%s)>", AS_NATIVE_OBJ(value)->name);                break;
  case OBJ_INSTANCE: printf("<instance of %s>", AS_INSTANCE(value)->klass->name->chars); break;
  default: printf("Unknown object[%p]: %d", value.payload.object, OBJECT_TYPE(value));   break;
  }
#ifdef CLOX_OBJECT_TYPE
  printf("\">");
#endif // CLOX_OBJECT_TYPE
}

void object_delete(Object* object) {
  if (!object) {
    printf("Delete Object NULL detected.\n");
    exit(90);
  }
#ifdef CLOX_ODEL_TRACE
  printf("%p DELETE(%s): ", object, strobjtype(object->type));
  value_oprint(OBJECT_VAL(object));
  putchar(10);
#endif
  switch ( object->type ) {
  case OBJ_BOUND_METHOD: FREE(ObjectBoundMethod, object);        break;
  case OBJ_NATIVE: FREE(ObjectNative, object);                   break;
  case OBJ_UPVALUE: FREE(ObjectUpvalue, object);                 break;
  case OBJ_STRING: {
    ObjectString* string = (ObjectString*)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjectString, object);                                  break;
  }
  case OBJ_FUNCTION:
    chunk_delete(&((ObjectFunction*)object)->chunk);
    FREE(ObjectFunction, object);                                break;
  case OBJ_CLOSURE: {
    ObjectClosure* c = (ObjectClosure*)object;
    FREE_ARRAY(ObjectUpvalue*, c->upvalues, c->upvalue_count);
    FREE(ObjectClosure, object);                                 break;
  }
  case OBJ_CLASS:
    table_delete(&((ObjectClass*)object)->methods);
    FREE(ObjectClass, object);                                   break;
  case OBJ_INSTANCE:
    table_delete(&((ObjectInstance*)object)->fields);
    FREE(ObjectInstance, object);                                break;
  default: printf("Deleting unknown object: %p\n", object);      break;
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
