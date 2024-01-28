#ifndef _CLOX_COMPILER_H
#define _CLOX_COMPILER_H

#include "common.h"
#include "scanner.h"

CLOX_BEG_DECLS

void compile(const char *source) {
  scanner_init(source);
  int line = -1;
  Token token;
  for (;;) {
    token = scan();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    }
    else printf("   | ");
    printf("%2d '%.*s'\n", token.type, token.length, token.start);
    if (token.type == TOKEN_EOF) break;
  }
}

CLOX_END_DECLS

#endif //_CLOX_COMPILER_H