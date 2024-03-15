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
} Vm;

typedef enum {
  INTERPRET_OKAY,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

Vm vm;

void stack_push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

bool intern_string(ObjectString* string) {
  return table_set(&vm.strings, string, NIL_VAL);
}

ObjectString* table_find_istring(char* payload, int size, uint32_t hash) {
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
  stack_push(OBJECT_VAL(copy_string(name, (int)strlen(name))));
  stack_push(OBJECT_VAL(new_native(function)));
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
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
  }
}

ObjectString* take_string(char* chars, int length) {
  uint32_t hash = hash_string(chars, length);
  ObjectString* string = table_find_string(&vm.strings, chars, length, hash);
  if ( string != NULL ) {
    FREE_ARRAY(char, chars, length + 1);
    return string;
  }
  return allocate_string(chars, length, hash);
}

void concatenate_string() {
  ObjectString* b = AS_STRING(stack_pop());
  ObjectString* a = AS_STRING(stack_pop());
  int length = a->length + b->length;
  char* payload = ALLOCATE(char, length + 1);
  payload[length] = '\0';
  memcpy(payload, a->chars, a->length);
  memcpy(payload + a->length, b->chars, b->length);
  ObjectString* result = take_string(payload, length);
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
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

void close_upvalues(Value *last) {
  ObjectUpvalue *upvalue;
  while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
    upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

InterpretResult run() {
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
        TOP_FRAME()->closure->upvalues[READ_BYTE()];                        break;
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
      if ( !table_get(&vm.globals, name, &value) ) {
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
  ObjectFunction* function = compile(source);
  if ( function == NULL ) return INTERPRET_COMPILE_ERROR;
  // stack_push(OBJECT_VAL(function));
  ObjectClosure* closure = new_closure(function);
  // stack_pop();
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
  table_init(&vm.globals);
  table_init(&vm.strings);
  vm.objects = NULL;
  reset_stack();
  setup_lox_native();
}

void vm_intern_string(ObjectString* string) {
  table_set(&vm.strings, string, NIL_VAL);
}

void vm_delete() {
  table_delete(&vm.globals);
  table_delete(&vm.strings);
  objects_delete(vm.objects);
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
