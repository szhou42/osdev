#ifndef PRINTF_H
#define PRINTF_H
#include <stdarg.h>

int is_format_letter(char c);

void vsprintf(char * str, void (*putchar)(char), const char * format, va_list arg);

void vsprintf_helper(char * str, void (*putchar)(char), const char * format, uint32_t * pos, va_list arg);

void printf(const char * s, ...);

#endif
