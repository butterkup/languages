#ifndef _CLOX_NATIVES_H
#define _CLOX_NATIVES_H

#include "common.h"
#include "value.h"
#include "time.h"
#include "object.h"
#include "unistd.h"

CLOX_BEG_DECLS

long double start_time();
void define_native(const char*, NativeFn);

Value clock_native(int arg_count, Value* args) {
  if ( arg_count != 0 )
    return ERROR_VAL("Did not expect any arguments.");
  // My clock.
  return NUMBER_VAL((long double)time(NULL) - start_time());
  // For some reason clock doesn't work if I call sleep_native
  // return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value exit_native(int arg_count, Value* args) {
  if ( arg_count > 1 )
    return ERROR_VAL("exit expected one integer argument.");
  if ( arg_count == 0 ) exit(0);
  else if ( IS_NUMBER(*args) ) {
    double code = AS_NUMBER(*args);
    if ( code != (int)code )
      return ERROR_VAL("Exit code must be a positive integer.");
    exit((int)code);
  } else return ERROR_VAL("Expected an integer exit code.");
  return NIL_VAL;
}

Value sleep_native(int arg_count, Value* args) {
  if ( arg_count != 1 )
    return ERROR_VAL("Expected one integer argument");
  else if ( IS_NUMBER(*args) ) {
    double seconds = AS_NUMBER(*args);
    if ( seconds < 0 )
      return ERROR_VAL("Seconds must be positive integer.");
    if ( seconds != (uint32_t)seconds )
      return ERROR_VAL("Expected seconds to be an integer");
    sleep((uint32_t)seconds);
  } else return ERROR_VAL("Expected an integer seconds argument.");
  return NIL_VAL;
}

void setup_lox_native() {
  define_native("exit", exit_native);
  define_native("clock", clock_native);
  define_native("sleep", sleep_native);
}

CLOX_END_DECLS

#endif //_CLOX_NATIVES_H