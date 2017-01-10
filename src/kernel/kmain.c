#include <multiboot.h>
#include <system.h>
#include <printf.h>
#include <gdt.h>
#include <idt.h>
#include <tss.h>
#include <vga.h>
#include <timer.h>
#include <asciiart.h>
#include <pmm.h>
#include <paging.h>
#include <kheap.h>
#include <pci.h>
#include <ata.h>
#include <vfs.h>
#include <string.h>
#include <ext2.h>
#include <process.h>
#include <usermode.h>
#include <syscall.h>
#include <elf_loader.h>
#include <vesa.h>
#include <bitmap.h>
#include <compositor.h>
#include <mouse.h>
#include <keyboard.h>
#include <font.h>
#include <rtc.h>
#include <rtl8139.h>
#include <ethernet.h>
#include <ip.h>
#include <serial.h>
#include <blend.h>


extern uint8_t * bitmap;
extern ata_dev_t primary_master;
extern datetime_t current_datetime;

#define MSIZE 48 * M
#define GUI_MODE 1
#define NETWORK_MODE 0

uint32_t sse_available();
void sse_init();

uint32_t len = 0;

void routineC() {
    for(int i = 0; i < 1000; i++) {

    }
}

void routineA() {
    for(int i = 0; i < 1000; i++) {
        if(i == 800) {
            i = 0;
            len = strlen("szhou42 szhou42 szhou42 szhou42");
        }
    }
    for(;;);
}

void routineB() {
    for(int i = 0; i < 1000; i++) {
        if(i == 800) {
            i = 0;
            len = strlen("szhou42 szhou42 szhou42 szhou42");
        }
    }
    for(;;);
}

void kernel_thread() {
    create_process_from_routine(routineA, "routineA");
    qemu_printf("Routine A created\n");
    create_process_from_routine(routineB, "routineB");
    qemu_printf("Routine B created\n");
    for(;;);
}

int kmain(multiboot_info_t * mb_info) {

    video_init();
    qemu_printf("%s\n", simpleos_logo);

    // Initialize everything
    qemu_printf("Initializing video(text mode 80 * 25)...\n");

    qemu_printf("Initializing gdt, idt and tss...\n");
    gdt_init();
    idt_init();
    tss_init(5, 0x10, 0);

    qemu_printf("Initializing physical memory manager...\n");
    pmm_init(1096 * M);

    qemu_printf("Initializing paging...\n");
    paging_init();

    qemu_printf("Initializing kernel heap...\n");
    kheap_init(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, KHEAP_MAX_ADDRESS);

    qemu_printf("Initializing timer...\n");
    timer_init();

    process_init();

    qemu_printf("Initializing system calls and enter...\n");
    syscall_init();

    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    tss_set_stack(0x10, esp);

    create_process_from_routine(kernel_thread, "kernel");

    for(;;);
    return 0;
}
