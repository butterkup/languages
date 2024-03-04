#ifndef _CLOX_DEBUG_H
#define _CLOX_DEBUG_H

#include "common.h"
#include "chunk.h"

CLOX_BEG_DECLS

int byte_instruction(Chunk *, int);
int simple_instruction(Chunk *, int);
int constant_instruction(Chunk *, int);
int jump_instruction(Chunk *, int, int);
int disassemble_instruction(Chunk *, int);
void disassemble_chunk(Chunk *, const char *);

void disassemble_chunk(Chunk *chunk, const char *name) {
  const char *line = "================";
  printf("%s[ %s ]%s\n", line, name, line);
  for (int offset = 0; offset < chunk->count;)
    offset = disassemble_instruction(chunk, offset);
}

int disassemble_instruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    printf("     ");
  else printf("%4d ", chunk->lines[offset]);
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_ADD:           return simple_instruction(chunk, offset);
  case OP_NIL:           return simple_instruction(chunk, offset);
  case OP_NOT:           return simple_instruction(chunk, offset);
  case OP_POP:           return simple_instruction(chunk, offset);
  case OP_LESS:          return simple_instruction(chunk, offset);
  case OP_TRUE:          return simple_instruction(chunk, offset);
  case OP_FALSE:         return simple_instruction(chunk, offset);
  case OP_EQUAL:         return simple_instruction(chunk, offset);
  case OP_PRINT:         return simple_instruction(chunk, offset);
  case OP_RETURN:        return simple_instruction(chunk, offset);
  case OP_NEGATE:        return simple_instruction(chunk, offset);
  case OP_DIVIDE:        return simple_instruction(chunk, offset);
  case OP_GREATER:       return simple_instruction(chunk, offset);
  case OP_MULTIPLY:      return simple_instruction(chunk, offset);
  case OP_SUBTRACT:      return simple_instruction(chunk, offset);
  case OP_SET_LOCAL:     return byte_instruction(chunk, offset);
  case OP_GET_LOCAL:     return byte_instruction(chunk, offset);
  case OP_JUMP:          return jump_instruction(chunk, 1, offset);
  case OP_JUMP_IF_FALSE: return jump_instruction(chunk, 1, offset);
  case OP_LOOP:          return jump_instruction(chunk, -1, offset);
  case OP_CONSTANT:      return constant_instruction(chunk, offset);
  case OP_SET_GLOBAL:    return constant_instruction(chunk, offset);
  case OP_GET_GLOBAL:    return constant_instruction(chunk, offset);
  case OP_DEFINE_GLOBAL: return constant_instruction(chunk, offset);
  default:
    printf("Unknown Instruction[%d]: '%s'\n", instruction, inst_print(instruction));
    return offset + 1;
  }
}

int simple_instruction(Chunk *chunk, int offset) {
  printf("%s\n", inst_print(chunk->code[offset])); return ++offset;
}

int constant_instruction(Chunk *chunk, int offset) {
  const char *name = inst_print(chunk->code[offset]);
  uint8_t constant = chunk->code[++offset];
  printf("%-16s %4d  '", name, constant);
  value_print(chunk->constants.values[constant]);
  printf("'\n");
  return ++offset;
}

int byte_instruction(Chunk *chunk, int offset) {
  const char *name = inst_print(chunk->code[offset]);
  uint8_t slot = chunk->code[++offset];
  printf("%-16s %4d\n", name, slot);
  return ++offset;
}

int jump_instruction(Chunk *chunk, int sign, int offset) {
  const char *name = inst_print(chunk->code[offset]);
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8) | (chunk->code[offset + 2]);
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

CLOX_END_DECLS

#endif //_CLOX_DEBUG_H
