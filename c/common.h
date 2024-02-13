#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>    /* bool */
#include <stddef.h>     /* NULL, size_t */
#include <stdlib.h>     /* realloc, free */
#include <stdint.h>     /* fixed width integer types! */
#include <stdio.h>      /* printf */

/* Meant for macros like `__LINE__`, so that they are expanded beforehand. */
#define lox_xtostring(expanded) #expanded
#define lox_stringify(toexpand) lox_xtostring(toexpand)
#define lox_loginfo(func)       __FILE__ ":" lox_stringify(__LINE__) ":" func

/* Ensure that the first vararg is a format string!!! */
#define lox_logfmt(func, ...)   lox_loginfo(func) ": " __VA_ARGS__

/* Writes string literal with debug/logging information to `stderr`. */
#define lox_dputs(func, info)   fputs(lox_logfmt(func, info), stderr);

/* Writes formatted string with debug and logging information to `stderr`. */
#define lox_dprintf(func, ...)  fprintf(stderr, lox_logfmt(func, __VA_ARGS__));

#endif /* CLOX_COMMON_H */
