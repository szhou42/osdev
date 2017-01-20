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
#include <udp.h>
#include <dhcp.h>
#include <serial.h>
#include <blend.h>


extern uint8_t * bitmap;
extern ata_dev_t primary_master;
extern datetime_t current_datetime;

#define MSIZE 48 * M
#define GUI_MODE 0
#define NETWORK_MODE 1

void test_bios32() {
    register16_t reg = {0};
    register16_t reg2 = {0};
    reg.ax = 0x4f01;
    reg.di = 0x9000;
    bios32_service(0x10, &reg, &reg2);
    print_reg16(&reg2);
}

uint32_t sse_available();
void sse_init();

int kmain(multiboot_info_t * mb_info) {

    video_init();
    qemu_printf("%s\n", simpleos_logo);

    // Initialize everything (green)
    set_curr_color(LIGHT_GREEN);
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

    qemu_printf("Initializing pci...\n");
    pci_init();

    qemu_printf("Initializing keyboard...\n");
    keyboard_init();

    qemu_printf("Initializing vfs, ext2 and ata/dma...\n");
    vfs_init();
    ata_init();
    ext2_init("/dev/hda", "/");


    qemu_printf("Initializing real time clock...\n");
    rtc_init();
    qemu_printf("Current date and time: %s\n", datetime_to_str(&current_datetime));
    //process_init();


    set_curr_color(WHITE);

    //qemu_printf("Initializing system calls and enter usermode...\n");
    //syscall_init();
    //vfs_db_listdir("/");

    //uint32_t esp;
    //asm volatile("mov %%esp, %0" : "=r"(esp));
    //tss_set_stack(0x10, esp);

#if NETWORK_MODE
    qemu_printf("Initializing network driver...\n");
    rtl8139_init();
    arp_init();

    uint8_t mac_addr[6];
    mac_addr[0] = 0xAA;
    mac_addr[1] = 0xBB;
    mac_addr[2] = 0xCC;
    mac_addr[3] = 0xDD;
    mac_addr[4] = 0xEE;
    mac_addr[5] = 0xFF;
    get_mac_addr(mac_addr);

    uint8_t ip_addr[6];
    ip_addr[0] = 10;
    ip_addr[1] = 0;
    ip_addr[2] = 2;
    ip_addr[3] = 15;
    char * str = "whats up udp!";
    //ethernet_send_packet(mac_addr, str, strlen(str), 0x0021);
    //ip_send_packet(ip_addr, str, strlen(str));
    dhcp_discover();
    while(gethostaddr(mac_addr) == 0);
    udp_send_packet(ip_addr, 1234, 1153, str, strlen(str));
    for(;;);
#endif

#if 0
    create_process("/test1.bin");
    bios32_init();
#endif

#if 0
    vesa_init();
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
            qemu_printf("something is wrong\n");
    }
#endif

#if GUI_MODE
    vesa_init();
    compositor_init();
    mouse_init();

    // Terminal
    window_t * red_w = window_create(get_super_window(), 20, 300, 750, 450, WINDOW_NORMAL, "window_black");
    window_add_title_bar(red_w);
    window_add_close_button(red_w);
    window_add_minimize_button(red_w);
    window_add_maximize_button(red_w);
    canvas_t canvas_red = canvas_create(red_w->width, red_w->height, red_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK + 1);
    draw_text(&canvas_red, "Terminal", 1, 42);
    window_add_round_corner(red_w);
    blend_windows(red_w);


    // File Browser
    window_t * green_w = window_create(get_super_window(), 100, 100, 400, 400, WINDOW_NORMAL, "window_classic");
    window_add_title_bar(green_w);
    qemu_printf("Adding close buttons\n");
    window_add_close_button(green_w);
    qemu_printf("Adding minimize buttons\n");
    window_add_minimize_button(green_w);
    qemu_printf("Adding maximize buttons\n");
    window_add_maximize_button(green_w);
    canvas_t canvas_green = canvas_create(green_w->width, green_w->height, green_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK + 1);
    draw_text(&canvas_green, "File browser", 1, 19);
    window_add_round_corner(green_w);
    blend_windows(green_w);

#if 0
    // Web browser
    window_t * blue_w =window_create(get_super_window(), 600, 100, 400, 400, WINDOW_NORMAL, "window_blue");
    window_add_title_bar(blue_w);
    window_add_close_button(blue_w);
    //window_add_minimize_button(blue_w);
    canvas_t canvas_blue = canvas_create(blue_w->width, blue_w->height, blue_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK + 1);
    draw_text(&canvas_blue, "Web Browser", 1, 19);
    window_add_round_corner(blue_w);
    blend_windows(blue_w);

    window_t * web_button = window_create(blue_w, 30, 30, 60, 30, WINDOW_CONTROL, "window_xp");
    canvas_t canvas_button = canvas_create(web_button->width, web_button->height, web_button->frame_buffer);
    draw_text(&canvas_button, "Button", 1, 1);
    blend_windows(web_button);

#endif

    // Top desktop bar
    window_t * bar_w = window_create(get_super_window(), 0, 0, 1024, 25, WINDOW_DESKTOP_BAR, "desktop_bar");
    canvas_t canvas_bar = canvas_create(bar_w->width, bar_w->height, bar_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK+1);
    draw_text(&canvas_bar, get_current_datetime_str(), 1, 115);
    blend_windows(bar_w);
    display_all_window();
    video_memory_update(NULL, 0);
    print_windows_depth();
#endif

    qemu_printf("drawing finish");
    set_curr_color(LIGHT_RED);

    qemu_printf("\nDone!\n");
    for(;;);
    return 0;
}
