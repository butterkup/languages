#ifndef _CLOX_VM_H
#define _CLOX_VM_H

#include "compiler.h"
#include "table.h"
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "natives.h"

CLOX_BEG_DECLS

#define TOP_FRAME() (&vm.frames[vm.frame_count - 1])
#define CHUNK() (TOP_FRAME()->closure->function->chunk)
#define VMIP() (TOP_FRAME()->ip)
#define READ_BYTE() (*VMIP()++)
#define READ_CONSTANT() (CHUNK().constants.values[READ_BYTE()])
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(Type, op)                                          \
  do {                                                               \
    if (!IS_NUMBER(stack_peek(0)) || !IS_NUMBER(stack_peek(1))) {    \
      runtime_error("Operands must be numbers.");                    \
      return INTERPRET_RUNTIME_ERROR;                                \
    }                                                                \
    double b = AS_NUMBER(stack_pop());                               \
    double a = AS_NUMBER(stack_pop());                               \
    stack_push(Type(a op b));                                        \
  } while(false)
#define READ_SHORT() (VMIP() += 2, (uint16_t)((VMIP()[-2] << 8) | VMIP()[-1]))
#define BOOL_COND() is_false(stack_peek(0))

typedef struct {
  // ObjectFunction *function;
  ObjectClosure* closure;
  uint8_t* ip;
  Value* slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frame_count;
  Value stack[STACK_MAX];
  Value* stack_top;
  Object* objects;
  Table strings;
  Table globals;
  ObjectUpvalue* open_upvalues;
  int gray_count;
  int gray_capacity;
  Object** gray_stack;
  ObjectString* init_string;
} Vm;

typedef enum {
  INTERPRET_OKAY,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

Vm vm;

ObjectString* take_string(char*, int);

void stack_push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

ObjectString* table_find_istring(const char* payload, int size, uint64_t hash) {
  return table_find_string(&vm.strings, payload, size, hash);
}

Value stack_pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

Value stack_peek(int distance) {
  return vm.stack_top[-1 - distance];
}

void reset_stack();
ObjectUpvalue* new_upvalue(Value*);
ObjectUpvalue* capture_upvalue(Value*);

void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc(10, stderr);

  CallFrame* frame;
  size_t instruction;
  ObjectFunction* function;

  for ( int i = vm.frame_count - 1; i >= 0; --i ) {
    frame = &vm.frames[i];
    function = frame->closure->function;
    instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if ( function->name == NULL ) fprintf(stderr, "script\n");
    else fprintf(stderr, "%s()\n", function->name->chars);
  }
  reset_stack();
}

void define_native(const char* name, NativeFn function) {
  stack_push(OBJECT_VAL(copy_string(name, strlen(name))));
  stack_push(OBJECT_VAL(new_native(function, name)));
  ObjectString* str = AS_STRING(stack_peek(1));
  // printf("String['%s']: %p\n", str->chars, str);
  table_set(&vm.globals, AS_STRING(stack_peek(1)), stack_peek(0));
  stack_pop();
  stack_pop();
}

bool is_false(Value value) {
  return IS_NIL(value) ||
    ((IS_NUMBER(value)) && AS_NUMBER(value) == 0) ||
    ((IS_STRING(value)) && (AS_STRING(value))->length == 0) ||
    ((IS_BOOL(value)) && AS_BOOL(value) == false);
}

bool values_equal(Value a, Value b) {
  if ( a.type != b.type ) return false;
  switch ( a.type ) {
  case VAL_NIL: return true;
  case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJECT: return AS_OBJECT(a) == AS_OBJECT(b);
  default: printf("ValuesEqual: type=%d not defined.\n", a.type);
  }
  return false;
}

ObjectString* take_string(char* chars, int length) {
  uint64_t hash = hash_string(chars, length);
  ObjectString* string = table_find_string(&vm.strings, chars, length, hash);
  if ( string != NULL ) {
    FREE_ARRAY(char, chars, length + 1);
    return string;
  }
  return allocate_string(chars, length, hash);
}

void concatenate_string() {
  ObjectString* b = AS_STRING(stack_peek(0));
  ObjectString* a = AS_STRING(stack_peek(1));
  int length = a->length + b->length;
  char* payload = ALLOCATE(char, length + 1);
  payload[length] = '\0';
  memcpy(payload, a->chars, a->length);
  memcpy(payload + a->length, b->chars, b->length);
  ObjectString* result = take_string(payload, length);
  stack_pop();
  stack_pop();
  stack_push(OBJECT_VAL(result));
}

bool call_function(ObjectClosure* closure, int arg_count) {
  ObjectFunction* function = closure->function;
  if ( arg_count != function->arity ) {
    runtime_error(
      "Expected %d arguments but got %d.",
      function->arity, arg_count
    ); return false;
  }
  if ( vm.frame_count == FRAMES_MAX ) {
    runtime_error("Call stack overflow.");
    return false;
  }
  CallFrame* frame = &vm.frames[vm.frame_count++];
  // frame->function = function;
  frame->closure = closure;
  frame->ip = function->chunk.code;
  frame->slots = vm.stack_top - arg_count - 1;
  return true;
}

bool call_value(Value callee, int arg_count) {
  if ( IS_OBJECT(callee) ) switch ( OBJECT_TYPE(callee) ) {
    // case OBJ_FUNCTION: return call_function(AS_FUNCTION(callee), arg_count);
  case OBJ_CLOSURE:  return call_function(AS_CLOSURE(callee), arg_count);
  case OBJ_NATIVE: {
    NativeFn native = AS_NATIVE(callee);
    Value result = native(arg_count, vm.stack_top - arg_count);
    vm.stack_top -= arg_count;
    stack_push(result); return true;
  }
  case OBJ_CLASS: {
    ObjectClass* klass = AS_CLASS(callee);
    vm.stack_top[-arg_count - 1] = OBJECT_VAL(new_instance(klass));
    Value initializer;
    if ( table_get(&klass->methods, vm.init_string, &initializer) )
      return call_function(AS_CLOSURE(initializer), arg_count);
    else if ( arg_count ) {
      runtime_error("Expected 0 arguments but got %d.", arg_count);
      return false;
    }
    return true;
  }
  case OBJ_BOUND_METHOD: {
    ObjectBoundMethod* bound_method = AS_BOUND_METHOD(callee);
    vm.stack_top[-arg_count - 1] = bound_method->receiver;
    return call_function(bound_method->method, arg_count);
  }
  }
  runtime_error("Can only call functions, classes and bound methods.");
  return false;
}

void close_upvalues(Value* last) {
  ObjectUpvalue* upvalue;
  while ( vm.open_upvalues != NULL && vm.open_upvalues->location >= last ) {
    upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

void define_method(ObjectString* method_name) {
  Value method = stack_peek(0);
  ObjectClass* klass = AS_CLASS(stack_peek(1));
  table_set(&klass->methods, method_name, method);
  stack_pop();
}

bool bind_method(ObjectClass* klass, ObjectString* name) {
  Value method;
  if ( !table_get(&klass->methods, name, &method) ) return false;
  ObjectBoundMethod* bound_method = new_bound_method(stack_peek(0), AS_CLOSURE(method));
  stack_pop(); // Instance
  stack_push(OBJECT_VAL(bound_method));
  return true;
}

bool invoke_from_class(ObjectClass* klass, ObjectString* method_name, int arg_count) {
  Value method;
  if (!table_get(&klass->methods, method_name, &method)) {
    runtime_error("Undefined property '%s'.", method_name->chars);
    return false;
  }
  return call_function(AS_CLOSURE(method), arg_count);
}

bool invoke_property(ObjectString* property, int arg_count) {
  Value receiver = stack_peek(arg_count);
  if (!IS_INSTANCE(receiver)) {
    runtime_error("Only instances have properties.");
    return false;
  }
  ObjectInstance* instance = AS_INSTANCE(receiver);
  Value field;
  if(table_get(&instance->fields, property, &field)) {
    vm.stack_top[-arg_count - 1] = field;
    return call_value(field, arg_count);
  }
  return invoke_from_class(instance->klass, property, arg_count);
}

InterpretResult run() {
  // puts("--- RUNNING ---");
#ifdef CLOX_AINST_TRACE
  disassemble_chunk(&CHUNK(), "All Instructions");
#endif // CLOX_AINST_TRACE
#ifndef CLOX_DRY_RUN
  uint8_t instruction;
  for ( ;;) {
#ifdef CLOX_STACK_TRACE
    printf("STACK [");
    for ( Value* slot = vm.stack; slot < vm.stack_top; slot++ ) {
      value_print(*slot);
      if ( slot + 1 != vm.stack_top )
        printf(", ");
    }
    printf("]\n");
#endif
#ifdef CLOX_INST_TRACE
    disassemble_instruction(&CHUNK(), (int)(VMIP() - CHUNK().code));
#endif
    switch ( instruction = READ_BYTE() ) {
    case OP_NIL:      stack_push(NIL_VAL);                                    break;
    case OP_TRUE:     stack_push(BOOL_VAL(true));                             break;
    case OP_FALSE:    stack_push(BOOL_VAL(false));                            break;
    case OP_CONSTANT: stack_push(READ_CONSTANT());                            break;
    case OP_LESS:     BINARY_OP(BOOL_VAL, < );                                break;
    case OP_GREATER:  BINARY_OP(BOOL_VAL, > );                                break;
    case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *);                               break;
    case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -);                               break;
    case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, / );                              break;
    case OP_NOT:      stack_push(BOOL_VAL(is_false(stack_pop())));            break;
    case OP_POP:      stack_pop();                                            break;
    case OP_PRINT:    value_print(stack_pop()); putchar(10);                  break;
    case OP_SET_LOCAL: TOP_FRAME()->slots[READ_BYTE()] = stack_peek(0);       break;
    case OP_GET_LOCAL: stack_push(TOP_FRAME()->slots[READ_BYTE()]);           break;
    case OP_JUMP_IF_FALSE: VMIP() += BOOL_COND() * READ_SHORT();              break;
    case OP_JUMP:          VMIP() += READ_SHORT();                            break;
    case OP_LOOP:          VMIP() -= READ_SHORT();                            break;
    case OP_CLOSE_UPVALUE: close_upvalues(vm.stack_top - 1); stack_pop();     break;
    case OP_CLASS: stack_push(OBJECT_VAL(new_class(READ_STRING())));          break;
    case OP_METHOD: define_method(READ_STRING());                             break;
    case OP_INVOKE: {
      ObjectString* property = READ_STRING();
      int arg_count = READ_BYTE();
      if(!invoke_property(property, arg_count))
        return INTERPRET_RUNTIME_ERROR;                                       break;
    }
    case OP_SET_PROPERTY: {
      if ( !IS_INSTANCE(stack_peek(1)) ) {
        runtime_error("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjectInstance* instance = AS_INSTANCE(stack_peek(1));
      table_set(&instance->fields, READ_STRING(), stack_peek(0));
      Value value = stack_pop();
      stack_pop(); // Instance
      stack_push(value);                                                      break;
    }
    case OP_GET_PROPERTY: {
      if ( !IS_INSTANCE(stack_peek(0)) ) {
        runtime_error("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjectInstance* instance = AS_INSTANCE(stack_peek(0));
      ObjectString* property = READ_STRING();
      Value value;
      if ( table_get(&instance->fields, property, &value) ) {
        stack_pop(); // Instance
        stack_push(value);                                                    break;
      }
      if ( bind_method(instance->klass, property) )                           break;
      runtime_error("Undefined property '%s'.", property->chars);
      return INTERPRET_RUNTIME_ERROR;
    }
    case OP_RETURN: {
      Value result = stack_pop();
      close_upvalues(TOP_FRAME()->slots);
      --vm.frame_count;
      if ( vm.frame_count == 0 ) {
        stack_pop();
        return INTERPRET_OKAY;
      }
      vm.stack_top = (TOP_FRAME() + 1)->slots;
      stack_push(result);                                                     break;
    }
    case OP_GET_UPVALUE:
      stack_push(*TOP_FRAME()->closure->upvalues[READ_BYTE()]->location);     break;
    case OP_SET_UPVALUE:
      *TOP_FRAME()->closure->upvalues[READ_BYTE()]->location = stack_peek(0); break;
    case OP_CLOSURE: {
      ObjectFunction* function = AS_FUNCTION(READ_CONSTANT());
      ObjectClosure* closure = new_closure(function);
      stack_push(OBJECT_VAL(closure));
      for ( int i = 0; i < closure->upvalue_count; ++i )
        closure->upvalues[i] = READ_BYTE() ?
        capture_upvalue(TOP_FRAME()->slots + READ_BYTE()) :
        TOP_FRAME()->closure->upvalues[READ_BYTE()];                          break;
    }
    case OP_CALL: {
      int arg_count = READ_BYTE();
      if ( !call_value(stack_peek(arg_count), arg_count) )
        return INTERPRET_RUNTIME_ERROR;                                       break;
    }
    case OP_SET_GLOBAL: {
      ObjectString* name = READ_STRING();
      if ( table_set(&vm.globals, name, stack_peek(0)) ) {
        table_del(&vm.globals, name);
        runtime_error("[Setter] Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }                                                                       break;
    }
    case OP_GET_GLOBAL: {
      ObjectString* name = READ_STRING();
      Value value;
      // printf("String['%s']: %p\n", name->chars, name);
      if ( !table_get(&vm.globals, name, &value) ) {
        table_print(&vm.globals);
        putchar(10);
        runtime_error("[Getter] Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      } stack_push(value);                                                    break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjectString* name = READ_STRING();
      table_set(&vm.globals, name, stack_peek(0));
      stack_pop();                                                            break;
    }
    case OP_ADD:
      if ( IS_STRING(stack_peek(0)) && IS_STRING(stack_peek(1)) )
        concatenate_string();
      else if ( IS_NUMBER(stack_peek(0)) && IS_NUMBER(stack_peek(1)) ) {
        double b = AS_NUMBER(stack_pop());
        double a = AS_NUMBER(stack_pop());
        stack_push(NUMBER_VAL(a + b));
      } else {
        runtime_error("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }                                                                       break;
    case OP_EQUAL: {
      Value b = stack_pop();
      Value a = stack_pop();
      stack_push(BOOL_VAL(values_equal(a, b)));                               break;
    }
    case OP_NEGATE:
      if ( !IS_NUMBER(stack_peek(0)) ) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      } stack_push(NUMBER_VAL(-AS_NUMBER(stack_pop())));                      break;
    }
  }
#endif // CLOX_DRY_RUN
  return INTERPRET_OKAY;
}

InterpretResult
interpret(const char* source) {
  // puts("--- INTERPRET ---");
  ObjectFunction* function = compile(source);
  if ( function == NULL ) return INTERPRET_COMPILE_ERROR;
  stack_push(OBJECT_VAL(function));
  ObjectClosure* closure = new_closure(function);
  stack_pop();
  stack_push(OBJECT_VAL(closure));
  call_value(OBJECT_VAL(closure), 0);
  return run();
}

void repl() {
  char line[1024];
  for ( ;;) {
    printf("> ");
    if ( !fgets(line, sizeof(line), stdin) ) {
      printf("\n");
      break;
    }
    interpret(line);
  }
}

char* read_file(const char* path) {
  // printf("--- READ FILE '%s' ---\n", path);
  FILE* file = fopen(path, "rb");
  if ( file == NULL ) {
    fputs("Cannot open file.\n", stderr);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  char* buffer = (char*)malloc(file_size + 1);
  if ( buffer == NULL ) {
    fputs("Cannot allocate enough memory.\n", stderr);
    exit(74);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if ( bytes_read < file_size ) {
    fputs("Could not load whole file.\n", stderr);
    exit(74);
  }
  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

int run_file(const char* path) {
  // puts("--- RUNNING FIlE ---");
  char* source = read_file(path);
  InterpretResult result = interpret(source);
  free(source);
  switch ( result ) {
  case INTERPRET_COMPILE_ERROR: return 65;
  case INTERPRET_RUNTIME_ERROR: return 70;
  default:
  case INTERPRET_OKAY:          return 0;
  }
}

void reset_stack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
  vm.open_upvalues = NULL;
}

void new_object(Object* object) {
  object->next = vm.objects;
  vm.objects = object;
}

ObjectFunction* new_function() {
  ObjectFunction* function = ALLOCATE_OBJECT(ObjectFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  function->upvalue_count = 0;
  chunk_init(&function->chunk);
  return function;
}

ObjectClosure* new_closure(ObjectFunction* function) {
  ObjectUpvalue** upvalues = ALLOCATE(ObjectUpvalue*, function->upvalue_count);
  for ( int i = 0; i < function->upvalue_count; ++i )
    upvalues[i] = NULL;
  ObjectClosure* closure = ALLOCATE_OBJECT(ObjectClosure, OBJ_CLOSURE);
  closure->upvalue_count = function->upvalue_count;
  closure->function = function;
  closure->upvalues = upvalues;
  return closure;
}

ObjectUpvalue* new_upvalue(Value* slot) {
  ObjectUpvalue* upvalue = ALLOCATE_OBJECT(ObjectUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

ObjectUpvalue* capture_upvalue(Value* slot) {
  ObjectUpvalue* prev = NULL, * upvalue = vm.open_upvalues;
  while ( upvalue != NULL && upvalue->location > slot ) { prev = upvalue; upvalue = upvalue->next; }
  if ( upvalue != NULL && upvalue->location == slot ) return upvalue;
  return *(prev == NULL ? &vm.open_upvalues : &prev->next) = new_upvalue(slot);
}

void vm_init() {
  vm.init_string = NULL;
  table_init(&vm.globals);
  table_init(&vm.strings);
  vm.objects = NULL;
  vm.gray_capacity = 0;
  vm.gray_count = 0;
  vm.gray_stack = NULL;
  reset_stack();
  vm.init_string = copy_string("init", 4);
  setup_lox_native();
}

void intern_string(ObjectString* string) {
  stack_push(OBJECT_VAL(string));
  table_set(&vm.strings, string, NIL_VAL);
  stack_pop();
  // printf("Interns: ");
  // for ( int idx = 0; idx < vm.strings.capacity; idx++ ) {
  //   Entry* e = vm.strings.entries + idx;
  //   if ( e->key ) printf("\"%.*s\" ", e->key->length, e->key->chars);
  // }
  // putchar(10);
}

void vm_delete() {
  vm.init_string = NULL;
  table_delete(&vm.globals);
  table_delete(&vm.strings);
  objects_delete(vm.objects);
  free(vm.gray_stack);
}

// GARBAGE COLLECTOR LIVES HERE: POOR CODE STRUCTURE.

void gc_mark_object(Object* object) {
  if ( !object ) return;
  if ( object->is_marked ) return;
#ifdef CLOX_GC_LOG
  printf("%p mark ", (void*)object);
  value_print(OBJECT_VAL(object));
  putchar(10);
#endif // CLOX_GC_LOG
  object->is_marked = true;
  if ( vm.gray_capacity < vm.gray_count + 1 ) {
    vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
    vm.gray_stack = realloc(vm.gray_stack, sizeof(Object*) * vm.gray_capacity);
    if ( vm.gray_stack == NULL ) exit(1);
  }
  vm.gray_stack[vm.gray_count++] = object;
}

void gc_mark_value(Value value) {
  if ( !IS_OBJECT(value) ) return;
  gc_mark_object(AS_OBJECT(value));
}

void gc_mark_table(Table* table) {
  Entry* entry;
  for ( int i = 0; i < table->capacity; ++i ) {
    entry = table->entries + i;
    gc_mark_object((Object*)entry->key);
    gc_mark_value(entry->value);
  }
}

void gc_mark_roots() {
  gc_mark_object((Object*)vm.init_string);
  for ( Value* slot = vm.stack; slot < vm.stack_top; ++slot )
    gc_mark_value(*slot);
  for ( int i = 0; i < vm.frame_count; ++i )
    gc_mark_object((Object*)vm.frames[i].closure);
  for ( ObjectUpvalue* upv = vm.open_upvalues; upv != NULL; upv = upv->next )
    gc_mark_object((Object*)upv);
  gc_mark_compiler_roots();
  gc_mark_table(&vm.globals);
}

void gc_mark_array(ValueArray* array) {
  for ( int i = 0; i < array->count; ++i )
    gc_mark_value(array->values[i]);
}

void gc_blacken_object(Object* object) {
#ifdef CLOX_GC_LOG
  printf("%p blacken ", (void*)object);
  value_print(OBJECT_VAL(object));
  putchar(10);
#endif // CLOX_GC_LOG
  switch ( object->type ) {
  case OBJ_NATIVE:
  case OBJ_STRING:                                                   break;
  case OBJ_UPVALUE: gc_mark_value(((ObjectUpvalue*)object)->closed); break;
  case OBJ_FUNCTION: {
    ObjectFunction* func = (ObjectFunction*)object;
    gc_mark_object((Object*)func->name);
    gc_mark_array(&func->chunk.constants);                           break;
  }
  case OBJ_CLOSURE: {
    ObjectClosure* closure = (ObjectClosure*)object;
    gc_mark_object((Object*)closure->function);
    for ( int i = 0; i < closure->upvalue_count; ++i )
      gc_mark_object((Object*)closure->upvalues[i]);                 break;
  }
  case OBJ_CLASS: {
    ObjectClass* klass = (ObjectClass*)object;
    gc_mark_table(&klass->methods);
    gc_mark_object((Object*)klass->name);                            break;
  }
  case OBJ_INSTANCE: {
    ObjectInstance* instance = (ObjectInstance*)object;
    gc_mark_object((Object*)instance->klass);
    gc_mark_table(&instance->fields);                                break;
  }
  case OBJ_BOUND_METHOD: {
    ObjectBoundMethod* bound_method = (ObjectBoundMethod*)object;
    gc_mark_value(bound_method->receiver);
    gc_mark_object((Object*)bound_method->method);                   break;
  }
  default: printf("Blackening Unknown Object: %p\n", object);
  }
}

void gc_table_remove_white(Table* table) {
  Entry* entry;
  for ( int i = 0; i < table->capacity; ++i ) {
    entry = table->entries + i;
    if ( entry->key && !entry->key->object.is_marked )
      table_del(table, entry->key);
  }
}

void gc_trace_references() {
  while ( vm.gray_count > 0 )
    gc_blacken_object(vm.gray_stack[--vm.gray_count]);
}

void gc_sweep() {
  Object* prev = NULL, * obj = vm.objects, * slot;
  while ( obj ) {
    if ( obj->is_marked ) {
      obj->is_marked = false;
      prev = obj; obj = obj->next;
      continue;
    }
#ifdef CLOX_GC_LOG
    printf("%p delete ", (void*)obj);
    value_print(OBJECT_VAL(obj));
    putchar(10);
#endif // CLOX_GC_LOG
    slot = obj; obj = obj->next;
    if ( prev ) prev->next = obj;
    else vm.objects = obj;
    object_delete(slot);
  }
}

void collect_garbage() {
#ifdef CLOX_NOGC
# ifdef CLOX_GC_LOG
  puts("GC Invoked.");
# endif
#else
# ifdef CLOX_GC_LOG
  puts("-- gc begin");
# endif // CLOX_GC_LOG
  gc_mark_roots();
  gc_trace_references();
  gc_table_remove_white(&vm.strings);
  gc_sweep();
# ifdef CLOX_GC_LOG
  puts("-- gc end");
# endif // CLOX_GC_LOG
#endif // CLOX_NOGC
}

#undef READ_CONSTANT
#undef READ_BYTE
#undef STACK_MAX
#undef BINARY_OP
#undef READ_STRING
#undef READ_SHORT
#undef BOOL_COND
#undef VMIP
#undef TOP_FRAME
#undef CHUNK

CLOX_END_DECLS

#endif //_CLOX_VM_H
