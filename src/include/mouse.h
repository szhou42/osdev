#ifndef MOUSE_H
#define MOUSE_H
#include <system.h>
#include <isr.h>
#include <printf.h>

#define MOUSE_LEFT_BUTTON(flag) (flag & 0x1)
#define MOUSE_RIGHT_BUTTON(flag) (flag & 0x2)
#define MOUSE_MIDDLE_BUTTON(flag) (flag & 0x4)

#define CURSOR_WIDTH 32
#define CURSOR_HEIGHT 32

typedef struct cursor{
    int x;
    int y;
    int width;
    int height;
    uint32_t * icon;
}cursor_t;

void mouse_init();

void mouse_handler(register_t * reg);

void mouse_wait(uint8_t a_type);

void mouse_write(uint8_t a_write);

uint8_t mouse_read();

void draw_mouse();

#endif
