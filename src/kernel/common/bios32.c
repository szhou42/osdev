#include <bios32.h>

idt_ptr_t real_idt_ptr;
idt_ptr_t real_gdt_ptr;

void (*rebased_bios32_helper)() = (void*)0x7c00;

/*
 * Prepare some descriptors for calling bios service from protected mode
 * We're using the easiest method : switch to real mode to call bios temporarily
 * So, prepare some 16bit global descriptors, and interrupt descriptors
 * */
void bios32_init() {
    // 16bit code segment
    gdt_set_entry(6, 0, 0xffffffff, 0x9A, 0x0f);
    // 16bit data segment
    gdt_set_entry(7, 0, 0xffffffff, 0x92, 0x0f);
    // gdt ptr
    real_gdt_ptr.base = (uint32_t)gdt_entries;
    real_gdt_ptr.limit = sizeof(gdt_entries) - 1;
    // idt ptr
    real_idt_ptr.base = 0;
    real_idt_ptr.limit = 0x3ff;
}

/*
 * Switch to real mode, do bios, then switch back to protected mode
 * need to move code to 0x7c00 for execution
 * the function that actually do all the mode switch and init work is in bios32_helper.asm (bios32_helper)
 * */
void bios32_service(uint8_t int_num, register16_t * in_reg, register16_t * out_reg) {
    void * new_code_base = (void*)0x7c00;

    // Copy relevant data to [0x8c00, ...] (gdt_entries, gdt_ptr, idt_ptr and so on)
    // And calculate the new address of these data so bios32_helper can reference them
    memcpy(&asm_gdt_entries, gdt_entries, sizeof(gdt_entries));

    real_gdt_ptr.base = (uint32_t)REBASE((&asm_gdt_entries));
    memcpy(&asm_gdt_ptr, &real_gdt_ptr, sizeof(real_gdt_ptr));

    memcpy(&asm_idt_ptr, &real_idt_ptr, sizeof(real_idt_ptr));

    memcpy(&asm_in_reg_ptr, in_reg, sizeof(register16_t));
    void * t = REBASE(&asm_in_reg_ptr);
    memcpy(&asm_intnum_ptr, &int_num, sizeof(uint8_t));

    // Copy bios32_helper's code to [0x7c00, 0x8c00] (I hope this is enough space)
    uint32_t size = (uint32_t)bios32_helper_end - (uint32_t)bios32_helper;
    memcpy(new_code_base, bios32_helper, size);
    if(size > 4096){
         PANIC("That's not enough memory for bios32_helper, fix thix!\n");
    }

    // Now transfer control to bios32_helper, which basically does everything
    rebased_bios32_helper();

    // Write bios interrupt result register to out_reg
    // REBASE the asm_out_reg_ptr, memcpy!

    t = REBASE(&asm_out_reg_ptr);
    memcpy(out_reg, t, sizeof(register16_t));

    // Re-initialize the gdt and idt, otherwise bad thing will happen....
    gdt_init();
    idt_init();
}

