#include <system.h>
#include <idt.h>
#include <string.h>
#include <pic.h>
#include <isr.h>

idt_entry_t idt_entries[NUM_IDT_ENTRIES];
idt_ptr_t idt_ptr;

void idt_init() {
    memset(idt_entries, 0, sizeof(idt_entries));
    idt_ptr.base = (uint32_t)idt_entries;
    idt_ptr.limit = sizeof(idt_entries) - 1;
    pic_init();

    idt_set_entry( 0, (uint32_t)exception0 , 0x08, 0x8E);
    idt_set_entry( 1, (uint32_t)exception1 , 0x08, 0x8E);
    idt_set_entry( 2, (uint32_t)exception2 , 0x08, 0x8E);
    idt_set_entry( 3, (uint32_t)exception3 , 0x08, 0x8E);
    idt_set_entry( 4, (uint32_t)exception4 , 0x08, 0x8E);
    idt_set_entry( 5, (uint32_t)exception5 , 0x08, 0x8E);
    idt_set_entry( 6, (uint32_t)exception6 , 0x08, 0x8E);
    idt_set_entry( 7, (uint32_t)exception7 , 0x08, 0x8E);
    idt_set_entry( 8, (uint32_t)exception8 , 0x08, 0x8E);
    idt_set_entry( 9, (uint32_t)exception9 , 0x08, 0x8E);
    idt_set_entry(10, (uint32_t)exception10, 0x08, 0x8E);
    idt_set_entry(11, (uint32_t)exception11, 0x08, 0x8E);
    idt_set_entry(12, (uint32_t)exception12, 0x08, 0x8E);
    idt_set_entry(13, (uint32_t)exception13, 0x08, 0x8E);
    idt_set_entry(14, (uint32_t)exception14, 0x08, 0x8E);
    idt_set_entry(15, (uint32_t)exception15, 0x08, 0x8E);
    idt_set_entry(16, (uint32_t)exception16, 0x08, 0x8E);
    idt_set_entry(17, (uint32_t)exception17, 0x08, 0x8E);
    idt_set_entry(18, (uint32_t)exception18, 0x08, 0x8E);
    idt_set_entry(19, (uint32_t)exception19, 0x08, 0x8E);
    idt_set_entry(20, (uint32_t)exception20, 0x08, 0x8E);
    idt_set_entry(21, (uint32_t)exception21, 0x08, 0x8E);
    idt_set_entry(22, (uint32_t)exception22, 0x08, 0x8E);
    idt_set_entry(23, (uint32_t)exception23, 0x08, 0x8E);
    idt_set_entry(24, (uint32_t)exception24, 0x08, 0x8E);
    idt_set_entry(25, (uint32_t)exception25, 0x08, 0x8E);
    idt_set_entry(26, (uint32_t)exception26, 0x08, 0x8E);
    idt_set_entry(27, (uint32_t)exception27, 0x08, 0x8E);
    idt_set_entry(28, (uint32_t)exception28, 0x08, 0x8E);
    idt_set_entry(29, (uint32_t)exception29, 0x08, 0x8E);
    idt_set_entry(30, (uint32_t)exception30, 0x08, 0x8E);
    idt_set_entry(31, (uint32_t)exception31, 0x08, 0x8E);
    idt_set_entry(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_entry(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_entry(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_entry(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_entry(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_entry(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_entry(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_entry(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_entry(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_entry(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_entry(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_entry(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_entry(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_entry(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_entry(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_entry(47, (uint32_t)irq15, 0x08, 0x8E);
    idt_set_entry(128, (uint32_t)exception128, 0x08, 0x8E);

    idt_flush((uint32_t)&(idt_ptr));
    asm volatile("sti");
}

void idt_set_entry(int index, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entry_t * this = &idt_entries[index];
    this->base_lo = base & 0xFFFF;
    this->base_hi = (base >> 16) & 0xFFFF;
    this->always0 = 0;
    this->sel = sel;
    this->flags = flags | 0x60;
}
