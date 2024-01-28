#ifndef _CLOX_VM_H
#define _CLOX_VM_H

#include "compiler.h"
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"

CLOX_BEG_DECLS

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define STACK_MAX 256
#define BINARY_OP(op)   \
  do {                  \
    Value b = pop();    \
    Value a = pop();    \
    push(a op b);       \
  } while(false)

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stack_top;
} Vm;

typedef enum {
  INTERPRET_OKAY,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

Vm vm;

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

InterpretResult run() {
  uint8_t instruction;
  for (;;) {
#ifdef CLOX_DEBUG_TRACE
    printf("--> [");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      value_print(*slot);
      if (slot + 1 != vm.stack_top)
        printf(", ");
    }
    printf("]\n");
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    switch (instruction = READ_BYTE()) {
    case OP_RETURN:
      value_print(pop());
      printf("\n");
      return INTERPRET_OKAY;
    case OP_CONSTANT:
      push(READ_CONSTANT());
      break;
    case OP_NEGATE:
      push(-pop());
      break;
    case OP_ADD:
      BINARY_OP(+);
      break;
    case OP_MULTIPLY:
      BINARY_OP(*);
      break;
    case OP_DIVIDE:
      BINARY_OP(/ );
      break;
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;
    }
  }
}

InterpretResult
interpret(const char *source) {
  compile(source);
  return INTERPRET_OKAY;
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
    fputs("Cannot open file.", stderr);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fputs("Cannot allocate enough memory.", stderr);
    exit(74);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fputs("Could not load whole file.", stderr);
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

void vm_init() {
  reset_stack();
}

void vm_delete() { }


#undef READ_BYTE
#undef READ_CONSTANT
#undef STACK_MAX
#undef BINARY_OP

CLOX_END_DECLS

#endif //_CLOX_VM_H