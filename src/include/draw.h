#ifndef DRAW_H
#define DRAW_H
#include <system.h>

typedef struct rect {
    int x;
    int y;
    int width;
    int height;
}rect_t;

typedef struct rect_region {
    rect_t r;
    uint32_t * region;
}rect_region_t;

typedef struct canvas {
    int width;
    int height;
    uint32_t * framebuffer;
}canvas_t;

extern uint32_t * screen;
extern int screen_width;
extern int screen_height;


int get_pixel_idx(canvas_t * canvas, int x, int y);

void set_pixel(canvas_t * canvas, uint32_t val, int x, int y);

void remove_sharp_edges(canvas_t * canvas, int start_x, int start_y, int direction, int num_pixels, uint32_t end_alpha, uint32_t middle_alpha);

void round_corner_effect(canvas_t * canvas);

void set_fill_color(uint32_t color);

void draw_rect(canvas_t * t, int x, int y, int width, int height);

void draw_rect_pixels(canvas_t * canvas, rect_region_t * rect_region);

void draw_rect_clip_pixels(canvas_t * canvas, rect_region_t * rect_region, int rect_true_width);

void draw_rect_clip_pixels2(canvas_t * canvas, rect_region_t * rect_region, int rect_true_width, int rect_true_x, int rect_true_y);

void draw_line(canvas_t * canvas, int x1, int y1, int x2, int y2);

int is_line_overlap(int line1_x1, int line1_x2, int line2_x1, int line2_x2);

int is_rect_overlap(rect_t rect1, rect_t rect2);

rect_t find_rect_overlap(rect_t rect1, rect_t rect2);

int is_point_in_rect(int point_x, int point_y, rect_t * r);

rect_t rect_create(int x, int y, int width, int height);

canvas_t canvas_create(int width, int height, uint32_t * framebuffer);

#endif
