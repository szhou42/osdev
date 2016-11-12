#ifndef BITMAP_H
#define BITMAP_H
#include <vfs.h>
#include <printf.h>
#include <kheap.h>
#include <vesa.h>

typedef struct tagBITMAPFILEHEADER {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
}__attribute__((packed)) bmp_fileheader_t;

typedef struct tagBITMAPINFOHEADER {
    unsigned int biSize;
    long biWidth;
    long biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    long biXPelsPerMeter;
    long biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
}bmp_infoheader_t;

typedef struct bitmap {
    unsigned int width;
    unsigned int height;
    char * image_bytes;
    char * buf;
    unsigned int total_size;
    uint32_t bpp;
}bitmap_t;

typedef struct palette {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
}palette_t;

bitmap_t * bitmap_create(char * filename);

void bitmap_display(bitmap_t * bmp);

void bitmap_to_framebuffer(bitmap_t * bmp, uint32_t * frame_buffer);

void bitmap_to_framebuffer2(bitmap_t * bmp, uint32_t * frame_buffer);

#endif
