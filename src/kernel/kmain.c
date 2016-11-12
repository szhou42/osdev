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


extern uint8_t * bitmap;
extern ata_dev_t primary_master;

#define MSIZE 48 * M

void test_bios32() {
    register16_t reg = {0};
    register16_t reg2 = {0};
    reg.ax = 0x4f01;
    reg.di = 0x9000;
    bios32_service(0x10, &reg, &reg2);
    print_reg16(&reg2);
}

int kmain(multiboot_info_t * mb_info) {

    video_init();
    printf("%s\n", simpleos_logo);

    // Initialize everything (green)
    set_curr_color(LIGHT_GREEN);
    printf("Initializing video(text mode 80 * 25)...\n");

    printf("Initializing gdt, idt and tss...\n");
    gdt_init();
    idt_init();
    tss_init(5, 0x10, 0);

    printf("Initializing physical memory manager...\n");
    pmm_init(1096 * M);

    printf("Initializing paging...\n");
    paging_init();

    printf("Initializing kernel heap...\n");
    kheap_init(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, KHEAP_MAX_ADDRESS);

    printf("Initializing timer...\n");
    timer_init();

    printf("Initializing pci...\n");
    pci_init();

    printf("Initializing keyboard...\n");
    keyboard_init();

    printf("Initializing vfs, ext2 and ata/dma...\n");
    vfs_init();
    ata_init();
    ext2_init("/dev/hda", "/");

    //process_init();


    set_curr_color(WHITE);

    printf("Initializing system calls and enter usermode...\n");
    syscall_init();
    vfs_db_listdir("/");

    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    tss_set_stack(0x10, esp);

    //create_process("/test1.bin");
    //bios32_init();
    //test_bios32();

    vesa_init();
#if 0
    bitmap_t * bmp = bitmap_create("/wallpaper.bmp");
    bitmap_display(bmp);
#endif

#if 0
    vfs_node_t * f = file_open("/test.txt", 0);
    uint32_t size = vfs_get_file_size(f);
    char * buf = kcalloc(size, 1);
    vfs_read(f, 0, size, buf);
    for(int i = 0; i < size; i++) {
         if(buf[i] != 0x1)
            printf("something is wrong\n");
    }
#endif

#if 1
    compositor_init();
    window_t * red_w = window_create(get_super_window(), 300, 300, 400, 400, WINDOW_NORMAL, "window_red");
    window_add_headline(red_w, "red window");
    window_add_close_button(red_w);
    window_add_minimize_button(red_w);
    window_add_maximize_button(red_w);

    window_t * green_w = window_create(get_super_window(), 100, 100, 400, 400, WINDOW_NORMAL, "window_green");
    window_add_headline(green_w, "green window");
    window_add_close_button(green_w);
    window_add_minimize_button(green_w);
    window_add_maximize_button(green_w);

    window_t * blue_w =window_create(get_super_window(), 600, 100, 400, 400, WINDOW_NORMAL, "window_blue");
    window_add_headline(blue_w, "blue window");
    window_add_close_button(blue_w);
    window_add_minimize_button(blue_w);
    window_add_maximize_button(blue_w);

    window_create(get_super_window(), 0, 0, 1024, 25, WINDOW_NORMAL, "desktop_bar");
    display_all_window();
    show_character((void*)screen, VESA_COLOR_WHITE);
    mouse_init();
#endif

    printf("drawing finish");
    set_curr_color(LIGHT_RED);

    printf("\nDone!\n");
    for(;;);
    return 0;
}
