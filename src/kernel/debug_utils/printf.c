#include <string.h>
#include <system.h>
#include <printf.h>
#include <vga.h>

/*
 * Both printf and sprintf call this function to do the actual formatting
 * The only difference of printf and sprintf is, one writes to screen memory, and another writes to normal memory buffer
 * vsprintf should keeps track of current mem pointer to place next character(for printf, print_char alread keeps track of current screen posistion, so this is only true for sprintf)
 * */
void vsprintf(char * str, const char * format, va_list arg) {
    uint32_t pos = 0;
    vsprintf_helper(str, format, &pos, arg);
}


void vsprintf_helper(char * str, const char * format, uint32_t * pos, va_list arg) {
    char c;
    int sign, ival, sys;
    char buf[512];
    uint32_t uval;
    uint32_t size;

    memset(buf, 0, 512);

    while((c = *format++) != 0) {
        sign = 0;

        if(c == '%') {
            c = *format++;
            switch(c) {
                case 'd':
                case 'u':
                case 'x':
                case 'p':
                    size = 8;
                    if(c == 'd' || c == 'u')
                        sys = 10;
                    else
                        sys = 16;

                    uval = ival = va_arg(arg, int);
                    if(c == 'd' && ival < 0) {
                        sign= 1;
                        uval = -ival;
                    }
                    itoa(buf, uval, sys);
                    uint32_t len = strlen(buf);
                    if((c == 'x' || c == 'p') &&len < size) {
                        for(uint32_t i = 0; i < size - len; i++) {
                            buf[size - 1 - i] = buf[len - 1 - i];
                        }
                        for(uint32_t i = 0; i < size - len; i++) {
                            buf[i] = '0';
                        }
                    }
                    if(c == 'd' && sign) {
                        if(str) {
                            *(str + *pos) = '-';
                            *pos = *pos + 1;
                        }
                        else
                            print_char('-');
                    }
                    if(str) {
                        strcpy(str + *pos, buf);
                        *pos = *pos + strlen(buf);
                    }
                    else {
                        print_string(buf);
                    }
                    break;
                case 'c':
                    if(str) {
                        *(str + *pos) = (char)va_arg(arg, int);
                        *pos = *pos + 1;
                    }
                    else {
                        print_char((char)va_arg(arg, int));
                    }
                    break;
                case 's':
                    if(str) {
                        char * t = (char *) va_arg(arg, int);
                        strcpy(str + (*pos), t);
                        *pos = *pos + strlen(t);
                    }
                    else {
                        print_string((char *) va_arg(arg, int));
                    }
                    break;
                default:
                    break;
            }
            continue;
        }
        if(str) {
            *(str + *pos) = c;
            *pos = *pos + 1;
        }
        else {
            print_char(c);
        }

    }
}

/*
 * Simplified version of printf and sprintf
 *
 * printf is sprintf is very similar, except that sprintf doesn't print to screen
 * */

void printf(const char * s, ...) {
    va_list ap;
    va_start(ap, s);
    vsprintf(NULL, s, ap);
    va_end(ap);
}

void sprintf(char * buf, const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
}
