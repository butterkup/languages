#ifndef _CLOX_COMMON_H
#define _CLOX_COMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

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
#define UINT8_COUNT (UINT8_MAX + 1)
#define GC_HEAP_GROW_FACTOR 2

#ifndef GC_NEXT_INIT
# define GC_NEXT_INIT 1024 * 5
#endif // GC_NEXT_INIT

#ifdef CLOX_ALL_OPT
# ifndef SUPER_INVOKE_OPT
#  define SUPER_INVOKE_OPT
# endif // SUPER_INVOKE_OPT
# ifndef DOT_INVOKE_OPT
#  define DOT_INVOKE_OPT
# endif // DOT_INVOKE_OPT
#endif // CLOX_ALL_OPT

// #define CLOX_GC_STRESS
// #define CLOX_GC_LOG
// #define CLOX_NOGC
// #defin CLOX_VAR_NO_SELF_INIT
// #define CLOX_AINST_TRACE
// #define CLOX_STACK_TRACE
// #define CLOX_INST_TRACE
// #define CLOX_SCAN_TRACE
// #define CLOX_ODEL_TRACE
// #define CLOX_DRY_RUN

#endif //_CLOX_COMMON_H
