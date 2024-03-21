#ifndef _CLOX_NATIVES_H
#define _CLOX_NATIVES_H

#include "common.h"
#include "value.h"
#include "time.h"
#include "object.h"

CLOX_BEG_DECLS

void runtime_error(const char*, ...);
void define_native(const char*, NativeFn);

Value clock_native(int arg_count, Value* args) {
  if (arg_count != 0) runtime_error("Did not expect any arguments, got %d argument(s).", arg_count);
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value exit_native(int arg_count, Value *args) {
  if (arg_count > 1)
    runtime_error("exit expected integer argument, got %d argument(s).", arg_count);
  if (arg_count == 0) exit(0);
  else if (IS_NUMBER(*args)) exit(AS_NUMBER(*args));
  else runtime_error("Expected an integer exit code.");
  return NIL_VAL;
}

void setup_lox_native() {
  define_native("clock", clock_native);
  define_native("exit", exit_native);
}

CLOX_END_DECLS

#endif //_CLOX_NATIVES_H