#ifndef _CLOX_COMMON_H
#define _CLOX_COMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>

#ifdef __cplusplus
# define CLOX_BEG_DECLS extern "C" {
# define CLOX_END_DECLS }
#else
# define CLOX_BEG_DECLS
# define CLOX_END_DECLS
# ifndef false
#  define false 0
# endif
# ifndef true
#  define true 1
# endif
# ifndef bool
#  define bool int
# endif
#endif

// #define CLOX_STACK_TRACE
// #define CLOX_INST_TRACE
// #define CLOX_SCAN_TRACE

#endif //_CLOX_COMMON_H
