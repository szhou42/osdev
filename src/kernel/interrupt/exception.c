#include <system.h>
#include <isr.h>
#include <printf.h>
#include <vga.h>


char *exception_messages[32] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};


void final_exception_handler(register_t reg) {
    if(reg.int_no < 32) {
        set_curr_color(LIGHT_RED);
        printf("EXCEPTION: %s (err code is %x)\n", exception_messages[reg.int_no], reg.err_code);
        print_reg(&reg);
        set_curr_color(WHITE);
        for(;;);
    }
    if(interrupt_handlers[reg.int_no] != NULL) {
         isr_t handler = interrupt_handlers[reg.int_no];
         handler(&reg);
    }

}
