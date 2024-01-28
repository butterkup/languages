#ifndef _CLOX_VM_H
#define _CLOX_VM_H

#include "chunk.h"
#include "debug.h"
#include "value.h"

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()]);
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
      Value constant = READ_CONSTANT();
      push(constant);
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
      BINARY_OP(/);
      break;
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;
    }
  }
}

InterpretResult
interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = chunk->code;
  return run();
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

#endif //_CLOX_VM_H