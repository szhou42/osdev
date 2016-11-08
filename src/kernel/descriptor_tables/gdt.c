#include <gdt.h>

gdt_entry_t gdt_entries[NUM_DESCRIPTORS];
gdt_ptr_t   gdt_ptr;

void gdt_init() {
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = (uint32_t)gdt_entries;

    // NULL Segment, required
    gdt_set_entry(0, 0, 0, 0, 0);
    /* Kernel code, access(9A = 1 00 1 1 0 1 0)
        1   present
        00  ring 0
        1   always 1
        1   code segment
        0   can be executed by ring lower or equal to DPL,
        1   code segment is readable
        0   access bit, always 0, cpu set this to 1 when accessing this sector
    */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    /* Kernel data, access(92 = 1 00 1 0 0 1 0)
        Only differ at the fifth bit(counting from least insignificant bit), 0 means it's a data segment.
    */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    // User code and data segments, only differ in ring number(ring 3)
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush((uint32_t)(&gdt_ptr));
}

void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entry_t * this = &gdt_entries[index];

    // Low 16 bits, middle 8 bits and high 8 bits of base
    this->base_low = base & 0xFFFF;
    this->base_middle = (base >> 16) & 0xFF;
    this->base_high = (base >> 24 & 0xFF);

    /* Low 16 bits and high 4 bits of limit, since the high 4 bits of limits is between granularity and access, and we don't have 4 bit variable,
    low 4 bits of granularity actually represents high 4 bits of limits. It's weird, I know. */
    this->limit_low = limit & 0xFFFF;
    this->granularity = (limit >> 16) & 0x0F;

    this->access = access;

    // Only need the high 4 bits of gran
    this->granularity = this->granularity | (gran & 0xF0);
}

