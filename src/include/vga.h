#ifndef VGA_H
#define VGA_H
#include <system.h>
/*
 * Some constans defined for vga 80 * 25 mode
 * */

// Some colors
#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define BROWN           6
#define LIGHT_GREY      7
#define DARK_GREY       8
#define LIGHT_BLUE      9
#define LIGHT_GREEN     10
#define LIGHT_CYAN      11
#define LIGHT_RED       12
#define LIGHT_MAGENTA   13
#define LIGHT_BROWN     14
#define WHITE           15


// VGA 80 * 25 mode screen
#define SCREEN ((uint16_t*)(LOAD_MEMORY_ADDRESS + 0xB8000))
#define WIDTH 80
#define HEIGHT 25
#define DEFAULT_COLOR 0x0F

// Make a pixel(2 bytes), background is always black, a is a char, b is foreground color
#define PAINT(a,b) (((b & 0xF) << 8) | (a & 0xFF))

// Get pixel
#define PIXEL(x, y) SCREEN[y * 80 + x]

void video_init();

void print_string(char * s);

void print_char(char c);

void scroll();

void update_cursor();

void set_curr_color(uint8_t color);

uint8_t get_curr_color();

void clear();

#endif
