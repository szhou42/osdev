#include <isr.h>
#include <system.h>
#include <pic.h>
#include <printf.h>


// For both exceptions and irq interrupt
isr_t interrupt_handlers[256];


/*
 * Register a function as the handler for a certian interrupt number, both exception and irq interrupt can change their handler using this function
 * */
void register_interrupt_handler(int num, isr_t handler) {
    if(num < 256)
        interrupt_handlers[num] = handler;
}

void final_irq_handler(register_t reg) {
    if(interrupt_handlers[reg.int_no] != NULL) {
        isr_t handler = interrupt_handlers[reg.int_no];
        handler(&reg);
    }
    irq_ack(reg.int_no);
}

