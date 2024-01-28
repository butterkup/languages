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
  OP_DIVIDE,
  OP_RETURN,
  OP_NEGATE,
  OP_ADD,
} OpCode;

typedef struct {
  ValueArray constants;
  uint8_t *code;
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
  chunk_init(chunk);
}

int chunk_cappend(Chunk *chunk, Value value) {
  value_append(&chunk->constants, value);
  return chunk->constants.count - 1;
}

CLOX_END_DECLS

#endif //_CLOX_CHUNK_H
