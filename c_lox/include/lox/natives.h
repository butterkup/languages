#ifndef _CLOX_NATIVES_H
#define _CLOX_NATIVES_H

#include "common.h"
#include "value.h"
#include "time.h"
#include "object.h"

CLOX_BEG_DECLS

void define_native(const char *, NativeFn);

Value clock_native(int arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

void setup_lox_native() {
  define_native("clock", clock_native);
}

CLOX_END_DECLS

#endif //_CLOX_NATIVES_H