#ifndef GDT_H
#define GDT_H
#include <system.h>
// Number of global descriptors
#define NUM_DESCRIPTORS 8

// Gdt related structures
typedef struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

// Extern asm functions
extern void gdt_flush(uint32_t gdt_ptr);

// gdt functions
void gdt_init();
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

extern gdt_entry_t gdt_entries[NUM_DESCRIPTORS];

#endif
