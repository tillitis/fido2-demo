#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

typedef struct {
} FILE;

int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);

#endif
