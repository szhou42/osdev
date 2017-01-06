#ifndef GUI_COMPOSITOR_H
#define GUI_COMPOSITOR_H
#include <system.h>
#include <generic_tree.h>
#include <draw.h>

extern uint32_t fill_color;


#define WINDOW_SUPER   0
#define WINDOW_NORMAL  1
#define WINDOW_CONTROL 2
#define WINDOW_ALERT   3
#define WINDOW_DESKTOP_BAR   4

#define VESA_COLOR_WHITE 0x00ffffff
#define VESA_COLOR_BLACK 0x00000000
#define VESA_COLOR_RED   0x00ff0000
#define VESA_COLOR_GREEN 0x0000ff00
#define VESA_COLOR_BLUE  0x000000ff
#define VESA_COLOR_MAGENTA  0x00ff00ff

#define WINMSG_MOUSE 0
#define WINMSG_MOUSE_MOVE 0
#define WINMSG_MOUSE_LEFT_BUTTONDOWN 1
#define WINMSG_MOUSE_RIGHT_BUTTONDOWN 2
#define WINMSG_MOUSE_LEFT_BUTTONUP 3
#define WINMSG_MOUSE_RIGHT_BUTTONUP 4
#define WINMSG_MOUSE_LEFT_BUTTON_HELDDOWN 4
#define WINMSG_MOUSE_RIGHT_BUTTON_HELDDOWN 5
#define WINMSG_MOUSE_DOUBLECLICK 6

#define WINMSG_KEYBOARD 1

#define TITLE_BAR_HEIGHT 23

typedef struct point {
    int x;
    int y;
}point_t;

typedef struct window {
    // Name, optonal
    char name[20];

    // Coordinates of the top left coordination
    int x;
    int y;

    int width;
    int height;

    int original_width;
    int original_height;

    // Window fill color
    uint32_t fill_color;
    // Window border color
    uint32_t border_color;

    // Frame buffer
    uint32_t * frame_buffer;
    uint32_t * blended_framebuffer;
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
    int is_maximized;

    gtreenode_t * self;

    // Keep track of a list of windows that's under/above this window()
    list_t * under_windows;
    list_t * above_windows;

    int depth;

    // This rect maintains the rectangle portion of this window that may need to be redrawn next
    rect_t intersection_rect;
}window_t;

/*
 * The winmsg struct will be passed from mouse/keyboard driver or others, to the central window message handler
 * For example, clicking an area on window A will generate a winmsg of msg_type = mouse event, cursor_x and cursor_y = current position of the cursor
 * cursor_event_type = whether the mouse is clicking or double-clicking
 * The window message handler will act accordingly, based on what the message tells.
 * */
typedef struct winmsg {
    // Message type, like mouse click, or key press
    int msg_type;
    // Cursor event(click or double click)
    int sub_type;

    // Cursor position when the event happens
    int cursor_x;
    int cursor_y;

    int change_x;
    int change_y;

    // Keyboard key
    char key_pressed;

    // Involved window
    window_t * window;
}winmsg_t;

typedef struct window_dirty_region {
    window_t * w;
    rect_t rects[3];
    int len;
}window_dirty_region_t;

void calculate_intersections(rect_t * rect, window_t ** array, int * return_size);

void sort_windows_array(window_t ** array, int size);

void window_add_round_corner(window_t * w);

int is_moving();

window_t * get_focus_window();

void print_windows_depth();

void window_set_focus(window_t * w);

void copy_rect(uint32_t * dst, rect_t r);

void compositor_init();

window_t *  window_create(window_t * parent, int x, int y, int width, int height, int type, char * name);

void window_display(window_t * w, rect_t * rects, int size);

void display_all_window();

void display_recur(gtreenode_t * t);

void move_window(window_t * w, int x, int y);

window_t * get_super_window();

window_t * get_desktop_bar();

void blend_windows(window_t * w);

void blend_title_bar(window_t * w, int left);

void blend_window_rect(window_t * top_w, window_t * bottom_w);

void maximize_window(window_t * w);

void minimize_window(window_t * w);

void close_window(window_t * w);

void resize_window(window_t * w, int new_width, int new_height);

void add_under_windows(window_t * w);

void recur_add_under_windows(window_t * w, gtreenode_t * subroot);

window_t * query_window_by_point(int x, int y);

void repaint(rect_t r);

void find_possible_windows(int x, int y, gtreenode_t * subroot, window_t ** possible_windows, int * num);

uint32_t get_window_pixel(window_t * w, int x, int y);

int is_point_in_window(int x, int y, window_t * w);

canvas_t * get_screen_canvas();

void window_add_title_bar(window_t * w);

void window_add_close_button(window_t * w);

void window_add_minimize_button(window_t * w);

void window_add_maximize_button(window_t * w);

void window_message_handler(winmsg_t * msg);

point_t get_device_coordinates(window_t * w);

void get_device_rect(rect_t * out_rect, rect_t * relative_rect, point_t * p);

window_t * alertbox_create(window_t * parent, int x, int y, char * title, char * text);

point_t get_relative_coordinates(window_t * w, int x, int y);

point_t get_mouse_position_before_move();

int is_window_overlap(window_t * w1, window_t * w2);

void video_memory_update();

#endif
