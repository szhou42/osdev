#include <bitmap.h>
/*
 * Simmple bitmap library used in kernel
 *
 * */

bitmap_t * bitmap_create(char * filename) {
    bitmap_t * ret = kmalloc(sizeof(bitmap_t));
    //qemu_printf("Opening %s\n", filename);
    vfs_node_t * file = file_open(filename, 0);
    if(!file) {
        //qemu_printf("Fail to open %s\n", filename);
        return NULL;
    }

    uint32_t size = vfs_get_file_size(file);
    void * buf = kmalloc(size);
    //qemu_printf("Reading content of %s\n", filename);
    vfs_read(file, 0, size, buf);


    // Parse the bitmap
    bmp_fileheader_t * h = buf;
    unsigned int offset = h->bfOffBits;
    //qemu_printf("bitmap size: %u\n", h->bfSize);
    //qemu_printf("bitmap offset: %u\n", offset);

    bmp_infoheader_t * info = buf + sizeof(bmp_fileheader_t);

    ret->width = info->biWidth;
    ret->height = info->biHeight;
    ret->image_bytes= (void*)((unsigned int)buf + offset);
    ret->buf = buf;
    ret->total_size= size;
    ret->bpp = info->biBitCount;
    //qemu_printf("bitmap is %u x %u\n", ret->width, ret->height);
    //qemu_printf("file is here: %p\n", buf);
    //qemu_printf("image is here %p\n", ret->image_bytes);
    vfs_close(file);
    return ret;
}



void bitmap_display(bitmap_t * bmp) {
    if(!bmp) return;
    uint8_t * image = bmp->image_bytes;
    uint32_t * screen = (char*)0xfd000000;
    int j = 0;
    // Do copy
    for(int i = 0; i < bmp->height; i++) {
        // Copy the ith row of image to height - 1 - i row of screen, each row is of length width * 3
        char * image_row = image + i * bmp->width * 3;
        uint32_t * screen_row = (void*)screen + (bmp->height - 1 - i) * bmp->width * 4;
        j = 0;
        for(int k = 0; k < bmp->width; k++) {
            uint32_t b = image_row[j++] & 0xff;
            uint32_t g = image_row[j++] & 0xff;
            uint32_t r = image_row[j++] & 0xff;
            uint32_t rgb = ((r << 16) | (g << 8) | (b)) & 0x00ffffff;
            rgb = rgb | 0xff000000;
            screen_row[k] = rgb;
        }
    }
}

/*
 * Copy bitmap content to frame buffer
 * */
void bitmap_to_framebuffer(bitmap_t * bmp, uint32_t * frame_buffer) {
    if(!bmp) return;
    uint8_t * image = bmp->image_bytes;
    int j = 0;
    // Do copy
    for(int i = 0; i < bmp->height; i++) {
        // Copy the ith row of image to height - 1 - i row of frame buffer, each row is of length width * 3
        char * image_row = image + i * bmp->width * 3;
        //uint32_t * framebuffer_row = (void*)frame_buffer + (bmp->height - 1 - i) * bmp->width * 4;
        uint32_t * framebuffer_row = (void*)frame_buffer + (bmp->height - 1 - i) * bmp->width * 4;
        j = 0;
        for(int k = 0; k < bmp->width; k++) {
            uint32_t b = image_row[j++] & 0xff;
            uint32_t g = image_row[j++] & 0xff;
            uint32_t r = image_row[j++] & 0xff;
            uint32_t rgb = ((r << 16) | (g << 8) | (b)) & 0x00ffffff;
            rgb = rgb | 0xff000000;
            framebuffer_row[k] = rgb;
        }
    }
}
void bitmap_to_framebuffer2(bitmap_t * bmp, uint32_t * frame_buffer) {
    if(!bmp) return;
    uint8_t * image = bmp->image_bytes;
    int j = 0;
    // Do copy
    for(int i = 0; i < bmp->height; i++) {
        // Copy the ith row of image to height - 1 - i row of frame buffer, each row is of length width * 3
        char * image_row = image + i * bmp->width * 4;
        //uint32_t * framebuffer_row = (void*)frame_buffer + (bmp->height - 1 - i) * bmp->width * 4;
        uint32_t * framebuffer_row = (void*)frame_buffer + (bmp->height - 1 - i) * bmp->width * 4;
        j = 0;
        for(int k = 0; k < bmp->width; k++) {
            uint32_t b = image_row[j++] & 0xff;
            uint32_t g = image_row[j++] & 0xff;
            uint32_t r = image_row[j++] & 0xff;
            uint32_t a = image_row[j++] & 0xff;
            uint32_t rgba = ((a << 24) | (r << 16) | (g << 8) | (b));
            framebuffer_row[k] = rgba;
        }
    }
}
