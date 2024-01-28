#include <lox/all.h>
#include <stdio.h>

int main(int argc, char **argv) {
  vm_init();
  Chunk arr;
  chunk_init(&arr);
  // Bytecode for the expression
  // -((2 + 3) / 10)  = -0.5
  int constant = chunk_cappend(&arr, 2);
  chunk_append(&arr, OP_CONSTANT, 1);
  chunk_append(&arr, constant, 1);
  constant = chunk_cappend(&arr, 3);
  chunk_append(&arr, OP_CONSTANT, 1);
  chunk_append(&arr, constant, 1);
  chunk_append(&arr, OP_ADD, 1);
  constant = chunk_cappend(&arr, 10);
  chunk_append(&arr, OP_CONSTANT, 1);
  chunk_append(&arr, constant, 1);
  chunk_append(&arr, OP_DIVIDE, 1);
  chunk_append(&arr, OP_NEGATE, 1);
  chunk_append(&arr, OP_RETURN, 1);
  disassemble_chunk(&arr, "instructions");
  interpret(&arr);
  vm_delete();
  chunk_delete(&arr);
}
