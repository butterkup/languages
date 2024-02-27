#ifndef _CLOX_CHUNK_H
#define _CLOX_CHUNK_H

#include "memory.h"
#include "common.h"
#include "value.h"

CLOX_BEG_DECLS

typedef enum {
  OP_CONSTANT,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_GREATER,
  OP_DIVIDE,
  OP_RETURN,
  OP_NEGATE,
  OP_FALSE,
  OP_EQUAL,
  OP_TRUE,
  OP_LESS,
  OP_NIL,
  OP_ADD,
  OP_NOT,
} OpCode; // Instruction Bytes

#define INSTCS(inst) case OP##inst: return #inst + 1

const char *inst_print(uint8_t byte) {
  switch (byte)
  {
  INSTCS(_CONSTANT);
  INSTCS(_SUBTRACT);
  INSTCS(_MULTIPLY);
  INSTCS(_GREATER);
  INSTCS(_DIVIDE);
  INSTCS(_RETURN);
  INSTCS(_NEGATE);
  INSTCS(_FALSE);
  INSTCS(_EQUAL);
  INSTCS(_TRUE);
  INSTCS(_LESS);
  INSTCS(_NIL);
  INSTCS(_ADD);
  INSTCS(_NOT);
  }
  return "<UNKOWN_INST>";
}

#undef INSTCS

typedef struct {
  ValueArray constants;
  uint8_t *code; // Compiled Bytecode: from compile
  int capacity;
  int *lines;
  int count;
} Chunk;

void chunk_init(Chunk *chunk) {
  value_init(&chunk->constants);
  chunk->capacity = 0;
  chunk->lines = NULL;
  chunk->code = NULL;
  chunk->count = 0;
}

void chunk_append(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, capacity, chunk->capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, capacity, chunk->capacity);
  }
  chunk->lines[chunk->count] = line;
  chunk->code[chunk->count] = byte;
  chunk->count++;
}

void chunk_delete(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  value_delete(&chunk->constants);
}

int chunk_cappend(Chunk *chunk, Value value) {
  value_append(&chunk->constants, value);
  return chunk->constants.count - 1;
}

CLOX_END_DECLS

#endif //_CLOX_CHUNK_H
