#ifndef PRINTF_H
#define PRINTF_H
#include <stdarg.h>

void vsprintf(char * str, const char * format, va_list arg);
void vsprintf_helper(char * str, const char * format, uint32_t * pos, va_list arg);
void printf(const char * s, ...);
void sprintf(char * buf, const char * fmt, ...);

#endif
