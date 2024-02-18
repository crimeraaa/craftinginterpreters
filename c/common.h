#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>    /* bool */
#include <stddef.h>     /* NULL, size_t */
#include <stdlib.h>     /* realloc, free, exit */
#include <stdint.h>     /* fixed width integer types! */
#include <stdio.h>      /* FILE*, f* family, *printf family */
#include <string.h>     /*  */
#include <sysexits.h>   /* UNIX conventional exit codes. */

/* III:17.7 */
#define DEBUG_PRINT_CODE
/* III:15.1.2 */
#define DEBUG_TRACE_EXECUTION

/* III:22.1: our VM has a hard limit on how many locals can be in scope at once. */
#define UINT8_COUNT (UINT8_MAX + 1)

/* Meant for macros like `__LINE__`, so that they are expanded beforehand. */
#define lox_xtostring(expanded) #expanded
#define lox_stringify(toexpand) lox_xtostring(toexpand)
#define lox_loginfo(func)       __FILE__ ":" lox_stringify(__LINE__) ":" func

/* Ensure that the first vararg is a format string!!! */
#define lox_logfmt(func, ...)   lox_loginfo(func) ": " __VA_ARGS__

/* Writes string literal with debug/logging information to `stderr`. */
#define lox_dputs(func, info)   fputs(lox_logfmt(func, info), stderr);

/* Writes formatted string with debug/logging information to `stderr`. */
#define lox_dprintf(func, ...)  fprintf(stderr, lox_logfmt(func, __VA_ARGS__));

#endif /* CLOX_COMMON_H */
