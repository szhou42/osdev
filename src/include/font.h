#ifndef FONT_H
#define FONT_H

#include <system.h>
#include <draw.h>

#define FNT_FONTHEIGHT 8
#define FNT_FONTWIDTH 8

uint8_t * get_font_array();

uint32_t get_font_color();

void set_font_color(uint32_t color);

void draw_text(canvas_t * canvas, char * text, int start_x, int start_y);

#endif
