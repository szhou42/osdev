#ifndef SYSTEM_H
#define SYSTEM_H

// Some useful macro
#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

// Define some constants that (almost) all other modules need
#define PANIC(msg) panic(msg, __FILE__, __LINE__)
#define ASSERT(b) ((b) ? (void)0 : panic(#b, __FILE__, __LINE__))

// Our kernel now loads at 0xC0000000, so what low memory address such as 0xb800 you used to access, should be LOAD_MEMORY_ADDRESS + 0xb800
#define LOAD_MEMORY_ADDRESS 0xC0000000

#define NULL 0
#define TRUE 1
#define FALSE 0

#define K 1024
#define M (1024*K)
#define G (1024*M)

#define KDEBUG 1

typedef unsigned int uint32_t ;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// Register structs for interrupt/exception
typedef struct registers
{
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
}register_t;

// Register structs for bios service
typedef struct register16 {
    uint16_t di;
    uint16_t si;
    uint16_t bp;
    uint16_t sp;
    uint16_t bx;
    uint16_t dx;
    uint16_t cx;
    uint16_t ax;

    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;
    uint16_t ss;
    uint16_t eflags;
}register16_t;

// Defined in port_io.c
void outportb(uint16_t port, uint8_t val);
uint8_t inportb(uint16_t port);

uint16_t inports(uint16_t _port);
void outports(uint16_t _port, uint16_t _data);

uint32_t inportl(uint16_t _port);
void outportl(uint16_t _port, uint32_t _data);

// Defined in system.c
void panic(const char *message, const char *file, uint32_t line);
void print_reg(register_t * reg);
void print_reg16(register16_t * reg);
#endif
