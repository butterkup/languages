#ifndef _CLOX_VM_H
#define _CLOX_VM_H

#include "compiler.h"
#include "table.h"
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"

CLOX_BEG_DECLS

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define STACK_MAX 256
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

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stack_top;
  Object *objects;
  Table strings;
  Table globals;
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

bool intern_string(ObjectString *string) {
  return table_set(&vm.strings, string, NIL_VAL);
}

ObjectString *table_find_istring(char *payload, int size, uint32_t hash) {
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

void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script.\n", line);
  reset_stack();
}

bool is_false(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool values_equal(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
  case VAL_NIL: return true;
  case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJECT: return AS_OBJECT(a) == AS_OBJECT(b);
  }
}

ObjectString *take_string(char *chars, int length) {
  uint32_t hash = hash_string(chars, length);
  ObjectString *string = table_find_string(&vm.strings, chars, length, hash);
  if (string != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return string;
  }
  return allocate_string(chars, length, hash);
}

void concatenate_string() {
  ObjectString *b = AS_STRING(stack_pop());
  ObjectString *a = AS_STRING(stack_pop());
  int length = a->length + b->length;
  char *payload = ALLOCATE(char, length + 1);
  payload[length] = '\0';
  memcpy(payload, a->chars, a->length);
  memcpy(payload + a->length, b->chars, b->length);
  ObjectString *result = take_string(payload, length);
  stack_push(OBJECT_VAL(result));
}

InterpretResult run() {
  uint8_t instruction;
  for (;;) {
#ifdef CLOX_STACK_TRACE
    printf("STACK [");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      value_print(*slot);
      if (slot + 1 != vm.stack_top)
        printf(", ");
    }
    printf("]\n");
#endif
#ifdef CLOX_INST_TRACE
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    switch (instruction = READ_BYTE()) {
    case OP_RETURN:   return INTERPRET_OKAY;
    case OP_NIL:      stack_push(NIL_VAL);                         break;
    case OP_TRUE:     stack_push(BOOL_VAL(true));                  break;
    case OP_FALSE:    stack_push(BOOL_VAL(false));                 break;
    case OP_CONSTANT: stack_push(READ_CONSTANT());                 break;
    case OP_LESS:     BINARY_OP(BOOL_VAL, <);                      break;
    case OP_GREATER:  BINARY_OP(BOOL_VAL, >);                      break;
    case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *);                    break;
    case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -);                    break;
    case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, / );                   break;
    case OP_NOT:      stack_push(BOOL_VAL(is_false(stack_pop()))); break;
    case OP_POP:      stack_pop();                                 break;
    case OP_PRINT:    value_print(stack_pop()); putchar(10);       break;
    case OP_SET_LOCAL: vm.stack[READ_BYTE()] = stack_peek(0);      break;
    case OP_GET_LOCAL: stack_push(vm.stack[READ_BYTE()]);          break;
    case OP_SET_GLOBAL: {
      ObjectString *name = READ_STRING();
      if (table_set(&vm.globals, name, stack_peek(0))) {
        table_del(&vm.globals, name);
        runtime_error("[Setter] Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }                                                            break;
    }
    case OP_GET_GLOBAL: {
      ObjectString *name = READ_STRING();
      Value value;
      if (!table_get(&vm.globals, name, &value)) {
        runtime_error("[Getter] Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      } stack_push(value);                                         break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjectString *name = READ_STRING();
      table_set(&vm.globals, name, stack_peek(0));
      stack_pop();                                                 break;
    }
    case OP_ADD:
      if (IS_STRING(stack_peek(0)) && IS_STRING(stack_peek(1)))
        concatenate_string();
      else if (IS_NUMBER(stack_peek(0)) && IS_NUMBER(stack_peek(1))) {
        double b = AS_NUMBER(stack_pop());
        double a = AS_NUMBER(stack_pop());
        stack_push(NUMBER_VAL(a + b));
      } else {
        runtime_error("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }                                                            break;
    case OP_EQUAL: {
      Value b = stack_pop();
      Value a = stack_pop();
      stack_push(BOOL_VAL(values_equal(a, b)));                    break;
    }
    case OP_NEGATE:
      if (!IS_NUMBER(stack_peek(0))) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      } stack_push(NUMBER_VAL(-AS_NUMBER(stack_pop())));           break;
    }
  }
  return INTERPRET_OKAY;
}

InterpretResult
interpret(const char *source) {
  Chunk chunk;
  InterpretResult result;
  chunk_init(&chunk);
  if (compile(source, &chunk)) {
    vm.chunk = &chunk;
    vm.ip = chunk.code;
    result = run();
  }
  else result = INTERPRET_COMPILE_ERROR;
  chunk_delete(&chunk);
  return result;
}

void repl() {
  char line[1024];
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interpret(line);
  }
}

char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fputs("Cannot open file.\n", stderr);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fputs("Cannot allocate enough memory.\n", stderr);
    exit(74);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fputs("Could not load whole file.\n", stderr);
    exit(74);
  }
  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

void run_file(const char *path) {
  char *source = read_file(path);
  InterpretResult result = interpret(source);
  free(source);
  switch (result) {
  case INTERPRET_COMPILE_ERROR:
    exit(65);
  case INTERPRET_RUNTIME_ERROR:
    exit(70);
  }
}

void reset_stack() {
  vm.stack_top = vm.stack;
}

void new_object(Object *object) {
  object->next = vm.objects;
  vm.objects = object;
}

void vm_init() {
  table_init(&vm.globals);
  table_init(&vm.strings);
  vm.objects = NULL;
  reset_stack();
}

void vm_intern_string(ObjectString *string) {
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

CLOX_END_DECLS

#endif //_CLOX_VM_H
