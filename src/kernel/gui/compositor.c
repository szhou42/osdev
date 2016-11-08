#include <compositor.h>
#include <vesa.h>
#include <math.h>
#include <kheap.h>
#include <bitmap.h>
#include <printf.h>
#include <generic_tree.h>

gtree_t * windows_tree;

window_t *  window_create(window_t * parent, int x, int y, int width, int height, int type) {
    gtreenode_t * subroot;
    // Allocate spae and initialize
    window_t * w = kcalloc(sizeof(window_t), 1);
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->type = type;
    w->parent = parent;
    w->frame_buffer = kmalloc(width * height * 4);
    //memset(w->frame_buffer, 0xff, width * height * 4);
    w->fill_color = VESA_COLOR_WHITE;
    w->border_color= VESA_COLOR_BLACK;

    // Add it to the winodw tree
    if(type == WINDOW_SUPER)
        subroot = NULL;
    else if(type == WINDOW_NORMAL)
        subroot = windows_tree->root;
    else {
        // Every window except the super window should have a parent
        subroot = parent->self;
    }

    // Keep a reference to the treenode in window structure
    w->self = tree_insert(windows_tree, subroot, w);

    // Loop through all other windows in the tree, if overlap, add them to list w->under_windows
    add_under_windows(w);
    return w;
}

void add_under_windows(window_t * w) {
    recur_add_under_windows(w, windows_tree->root);
}

void recur_add_under_windows(window_t * w, gtreenode_t * subroot) {
    // Stop exploring any branches under w
    window_t * curr_w = subroot->value;
    if(curr_w == w) return;

    if(is_rect_overlap(rect_create(w->x, w->y, w->width, w->height), rect_create(curr_w->x, curr_w->y, curr_w->width, curr_w->height))) {
        list_insert_back(w->under_windows, curr_w);
        list_insert_back(curr_w->above_windows, w);
    }
    foreach(child, subroot->children) {
        recur_add_under_windows(w, child->val);
    }
}

/*
 * Correct way to do it:
 * draw it in the window specified in the parameter
 * Now, i m just going to draw it in the screen frame buffer
 * */
void window_display(window_t * w) {
    if(w->is_minimized) return;
    set_fill_color(w->border_color);


    // If the frame buffer is not null, also draw the frame buffer
    if(w->frame_buffer) {
        rect_region_t rect_reg;
        rect_reg.r.x = w->x;
        rect_reg.r.y = w->y;
        rect_reg.r.width = w->width;
        rect_reg.r.height = w->height;
        rect_reg.region = w->frame_buffer;
        draw_rect_pixels(&rect_reg);
    }
    else {
        // The horizontal line, starting from (w->x, w->y) to (w->x + w->width - 1, w->y)
        draw_line(w->x, w->y, w->x + w->width - 1, w->y);

        // The left line, starting from (w->x, w->y) to (w->x, w->y + w->height - 1)
        draw_line(w->x, w->y, w->x, w->y + w->height - 1);

        // The right line, starting from (w->x + w->width - 1, w->y) to (w->x + w->width - 1, w->y + w->height - 1)
        draw_line(w->x + w->width - 1, w->y, w->x + w->width - 1, w->y + w->height - 1);

        // The bottom line, starting from (w->x, w->y + w->height - 1) to (w->x + w->width - 1, w->y + w->height - 1)
        draw_line(w->x, w->y + w->height - 1, w->x + w->width - 1, w->y + w->height - 1);

        // Fill rect
        set_fill_color(w->fill_color);
        draw_rect(w->x + 1, w->y + 1, w->width - 2, w->height - 2);
    }
}

/*
 * Iterate through the whole tree and draw all window
 * This is slow! And ! It doesn't consider the case where windows overlap with each other
 * */
void display_all_window() {
    gtreenode_t * super = get_super_window()->self;
    // Display super window
    window_display(get_super_window());
    // Display sub window
    display_recur(super);
}

void display_recur(gtreenode_t * t) {
    foreach(child, t->children) {
        gtreenode_t * node = child->val;
        window_t * w = node->value;
        window_display(w);
        display_recur(node);
    }
}

window_t * get_super_window() {
    return windows_tree->root->value;
}

void set_window_fillcolor(window_t * w, uint32_t color) {
    w->fill_color = color;
}

void move_window(window_t * w, int x, int y) {
    w->x = x;
    w->y = y;
}

void minimize_window(window_t * w) {
    w->is_minimized = 1;
}

void close_window(window_t * w) {
    // Remove the window from tree

}

/*
 * Copy pixels in rect to dst
 * */
void copy_rect(uint32_t * dst, rect_t r) {
    for(int i = r.y; i < r.y + r.height; i++) {
        for(int j = r.x; j < r.x + r.width; j++) {
            *dst = screen[screen_width * i + j];
            //*dst = 0x00FF00FF;
            dst++;
        }
    }
}

/*
 * Helper function for repaint(), paint a pixel given its (x,y)
 * */
void paint_pixel(int x, int y) {

    // Very Important: Do not allocate any memory here, because this function is probably going to be called 99999 times per second, you don't want to call lots of mallocs in here
    // So, we will just assume that it's impossible that there are more than 20 windows stacked together and all contain one point
    window_t * possible_windows[20];

    // A list of windows that are standing on this pixel
    //list_t * possible_windows = list_create();
    find_possible_windows(x, y, windows_tree->root, possible_windows);
    // Now, we get a list of possible windows, find which one is on top (whichever has the most children is the one on top!)
    int max_children_num = 0;
    window_t * top_window = NULL;

    window_t ** window_list = possible_windows;
    while(*window_list != NULL) {
        window_t * w = *window_list;
        if(list_size(w->under_windows) > max_children_num) {
            max_children_num = list_size(w->under_windows);
            top_window = w;
        }
        window_list++;
    }

    if(top_window != NULL) {
        set_pixel(get_window_pixel(top_window, x, y), x, y);
    }
}

/*
 * Given a rectangle region, repaint the pixel it should have
 * */
void repaint(rect_t r) {
    for(int i = r.x; i < r.x + r.width; i++) {
        for(int j = r.y; j < r.y + r.height; j++) {
            paint_pixel(i, j);
        }
    }
}

/*
 * A recursive function for finding windows that contains the point (x, y)
 * */
void find_possible_windows(int x, int y, gtreenode_t * subroot, window_t ** possible_windows) {
    window_t * curr_w = subroot->value;

    if(is_point_in_window(x, y, curr_w)) {
        *possible_windows = curr_w;
        possible_windows++;
        *possible_windows = NULL;
    }
    foreach(child, subroot->children) {
        find_possible_windows(x, y, child->val, possible_windows);
    }
}

/*
 * Given x,y in a global frame buffer
 * Find the corresponding pixel in the window
 * */
uint32_t get_window_pixel(window_t * w, int x, int y) {
    // Transform (x,y) so that it's relative to the windows's frame buffer
    x = x - w->x;
    y = y - w->y;
    int idx = w->width * (w->height - y - 1) + x;
    return w->frame_buffer[idx];
}

int is_point_in_window(int x, int y, window_t * w) {
    return (x >= w->x && x < w->x + w->width) && (y >= w->y && y < w->y + w->height);
}




void compositor_init() {
    // Get linear frame buffer from vesa driver
    screen = vesa_get_lfb();
    screen_width = vesa_get_resolution_x();
    screen_height = vesa_get_resolution_y();

    // Initialize window tree to manage the windows to be created

    // Create a base window, this should be the parent of all windows(super window)
    windows_tree = tree_create();
    window_t * w = window_create(NULL, 0, 0, screen_width, screen_height, WINDOW_SUPER);
    set_window_fillcolor(w, VESA_COLOR_GREEN);

    bitmap_t * wallpaper = bitmap_create("/wallpaper.bmp");
    bitmap_to_framebuffer(wallpaper, w->frame_buffer);
    //w->frame_buffer = wallpaper->image_bytes;
    //memsetdw(w->frame_buffer, 0x00ff00ff,1024*768);

    // Default fill color is black
    fill_color =  (255 << 24) |(223 << 16) | (224 << 8) | 224;
}
