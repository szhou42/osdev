#ifndef GUI_COMPOSITOR_H
#define GUI_COMPOSITOR_H
#include <system.h>
#include <generic_tree.h>
#include <draw.h>

extern uint32_t fill_color;


#define WINDOW_SUPER   0
#define WINDOW_NORMAL  1
#define WINDOW_CONTROL 2

#define VESA_COLOR_WHITE 0x00ffffff
#define VESA_COLOR_BLACK 0x00000000
#define VESA_COLOR_RED   0x00ff0000
#define VESA_COLOR_GREEN 0x0000ff00
#define VESA_COLOR_BLUE  0x000000ff
#define VESA_COLOR_MAGENTA  0x00ff00ff

typedef struct window {
    // Canvas, put your pixels here and tell compositor to draw the window !
    void * canvas;

    // Coordinates of the top left coordination
    int x;
    int y;

    int width;
    int height;

    // Window fill color
    uint32_t fill_color;
    // Window border color
    uint32_t border_color;

    // Frame buffer
    uint32_t * frame_buffer;
    // Other properties of a window

    // Parent window
    struct window * parent;
    /*
     * Type
     *  - Super winodw, this is the parent of all, usually the desktop window
     *  - Normal window, all direct children of super window
     *  - Control window, controls like button, textbox, and so on
     * */
    int type;

    // Is this window active currently ?
    int is_focus;

    int is_minimized;

    gtreenode_t * self;
    // Keep track of a list of windows that's under/above this window()
    list_t * under_windows;
    list_t * above_windows;

}window_t;

void copy_rect(uint32_t * dst, rect_t r);

void compositor_init();

window_t *  window_create(window_t * parent, int x, int y, int width, int height, int type);

void window_display(window_t * w);

void display_all_window();

void display_recur(gtreenode_t * t);

void move_window(window_t * w, int x, int y);

window_t * get_super_window();

void minimize_window(window_t * w);

void close_window(window_t * w);

void add_under_windows(window_t * w);

void recur_add_under_windows(window_t * w, gtreenode_t * subroot);

void paint_pixel(int x, int y);

void repaint(rect_t r);

void find_possible_windows(int x, int y, gtreenode_t * subroot, window_t ** possible_windows);

uint32_t get_window_pixel(window_t * w, int x, int y);

int is_point_in_window(int x, int y, window_t * w);


#endif
