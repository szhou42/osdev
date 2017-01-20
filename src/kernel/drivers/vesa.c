#include <vesa.h>
#include <pic.h>

uint32_t curr_mode;


void vesa_memcpy24_32(uint24_t * dest, uint32_t * src, uint32_t count) {
    uint24_t t;
    uint32_t i;
    // How many bytes in total ?
    for(i = 0; i < count; i++) {
        t.integer = src[i];
        dest[i] = t;
    }
}
void vesa_memset_rgb(uint8_t * dest, uint32_t rgb, uint32_t count) {
    if(count % 3 != 0)
        count = count + 3 - (count % 3);
    uint8_t r = rgb & 0x00ff0000;
    uint8_t g = rgb & 0x0000ff00;
    uint8_t b = rgb & 0x000000ff;
    for(int i = 0; i < count; i++) {
        *dest++ = r;
        *dest++ = g;
        *dest++ = b;
    }
}

void * vesa_get_lfb() {
    mode_info_t mode_info = {0};
    vesa_get_mode((uint16_t)curr_mode, &mode_info);
    return (void*)mode_info.physbase;
}

int vesa_get_resolution_x() {
    mode_info_t mode_info = {0};
    vesa_get_mode((uint16_t)curr_mode, &mode_info);
    return (int)mode_info.resolutionX;
}

int vesa_get_resolution_y() {
    mode_info_t mode_info = {0};
    vesa_get_mode((uint16_t)curr_mode, &mode_info);
    return (int)mode_info.resolutionY;
}

void vesa_get_mode(uint16_t mode, mode_info_t * mode_info) {
    register16_t reg_in = {0};
    register16_t reg_out = {0};
    reg_in.ax = 0x4F01;
    reg_in.cx = mode;
    reg_in.di = 0x9000;
    bios32_service(BIOS_GRAPHICS_SERVICE, &reg_in, &reg_out);
    memcpy(mode_info, (void*)0x9000, sizeof(mode_info_t));
}

void vesa_set_mode(uint32_t mode) {
    register16_t reg_in = {0};
    register16_t reg_out = {0};
    reg_in.ax = 0x4F02;
    reg_in.bx = mode;
    bios32_service(BIOS_GRAPHICS_SERVICE, &reg_in, &reg_out);
    curr_mode = mode;
}

/*
 * Remember that, we can not assume 0x118 to be 1024*768*32, mode number could be different across different hardware implementation
 * So, we need to search for it by given height/width and bpp(bits per pixel)
 * */
uint32_t vesa_find_mode(uint32_t width, uint32_t height, uint32_t bpp) {
    vbe_info_t * vbe_info;
    mode_info_t * mode_info;
    register16_t reg_in = {0};
    register16_t reg_out = {0};

    // GET VBE info struct, put it in mem address 0x9000
    reg_in.ax = 0x4F00;
    reg_in.di = 0x9500;
    bios32_service(BIOS_GRAPHICS_SERVICE, &reg_in, &reg_out);
    memcpy(vbe_info, (void*)0x9500, sizeof(vbe_info_t));

    // Iterate through the video modes list
    uint16_t * mode_list = vbe_info->video_modes;
    uint16_t mode_number = *mode_list++;
    while(mode_number != 0xffff) {
        vesa_get_mode(mode_number, mode_info);
        // Found the right mode number
        if(mode_info->resolutionX == 1024 && mode_info->resolutionY == 768 && mode_info->bpp == 32)
            return mode_number;
        mode_number = *mode_list++;
    }
    // :(
    return 0;
}

uint32_t get_eflags() {
    return;
}
void vesa_init() {
    bios32_init();
    uint32_t eflags = 0;
    //uint32_t ret = vesa_find_mode(1024, 768, 32);
    //if(!ret)
    //    PANIC(":( Couldn't find the vesa mode\n");
    // Note!!! We're supposed to search to get the mode 0x144 ! But I've encounered some bugs in the vesa_find_mode() which cause the os the crash from time to time
    // Let's put that away, keep moving and fix it later :)
    vesa_set_mode(0x144 | 0x4000);
    //qemu_printf("Will there be any more irqs? eflags = %08x\n", eflags);
    //write_serial('w');
    //asm volatile("int $0x20");
    irq_ack(0x28);
    void * t = vesa_get_lfb();
    allocate_region(kpage_dir, (uint32_t)t, (uint32_t)(t + 1024*768*4), 1,1,1);
}
