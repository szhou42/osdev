#include <font.h>

char font_a[] = { 0x3c, 0xcc, 0xc3, 0xc3, 0xff, 0xc3, 0xc3 };

int RGBA_white = 0x00ff0000;

void show_character(char * VRAM, int color){

    int i, j;
    for (i = 0; i < 10; i++, VRAM += 1024) {
        char * row = VRAM;
        for (j = 1 << 8; j != 0;j = j >> 1) {
            if (font_a[i] & j)
                *row = RGBA_white;
            row++;
        }
    }
}
