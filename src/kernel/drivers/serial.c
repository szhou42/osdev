#include <serial.h>
#include <printf.h>
#include <stdarg.h>


// Receive

int serial_received() {
   return inportb(PORT_COM1 + 5) & 1;
}

char read_serial() {
   while (serial_received() == 0);
   return inportb(PORT_COM1);
}

// Send

int is_transmit_empty() {
   return inportb(PORT_COM1 + 5) & 0x20;
}

void write_serial(char a) {
   while (is_transmit_empty() == 0);
   outportb(PORT_COM1,a);
}

/*
* Print to QEMU's log
 * */
void qemu_printf(const char * s, ...) {
    va_list(ap);
    va_start(ap, s);
    vsprintf(NULL, write_serial, s, ap);
    va_end(ap);
}

void serial_init() {
   outportb(PORT_COM1 + 1, 0x00);
   outportb(PORT_COM1 + 3, 0x80);
   outportb(PORT_COM1 + 0, 0x03);
   outportb(PORT_COM1 + 1, 0x00);
   outportb(PORT_COM1 + 3, 0x03);
   outportb(PORT_COM1 + 2, 0xC7);
   outportb(PORT_COM1 + 4, 0x0B);
}
