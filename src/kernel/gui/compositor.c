#include <compositor.h>
#include <vesa.h>
#include <math.h>
#include <kheap.h>
#include <string.h>
#include <bitmap.h>
#include <printf.h>
#include <generic_tree.h>
#include <font.h>
#include <serial.h>
#include <timer.h>
#include <math.h>
#include <mouse.h>
#include <blend.h>


/*
 *  Why not to use black color in this os ??
 *  :( Unfortunately, a hack I used for displaying transparent-background cursor bitmap is that: I ignore all black colors, I view them as transparent color, because 0x00000000 in bmp file means
 *   transparent, this hack is ridiculous, I will fix this!!
 * */

gtree_t * windows_tree;
list_t * windows_list;
window_t * windows_array[200];
canvas_t canvas;

// Keep a reference to desktop bar
window_t * desktop_bar;

window_t * focus_w;

// Intermediate frame buffer
uint32_t * intermediate_framebuffer;

/*
 * In OS X's window's title bar, each row's color increments, instead of having one color for the entire title bar
 */
uint32_t title_bar_colors[TITLE_BAR_HEIGHT] = {0x00F3F3F3, 0x00EBEBEB, 0x00EBEBEB, 0x00E9E9E9, 0x00E7E7E7, 0x00E6E6E6, 0x00E5E5E5, 0x00E3E3E3, 0x00E2E2E2, 0x00E0E0E0, 0x00DFDFDF, 0x00DDDDDD,\
    0x00DCDCDC, 0x00DADADA, 0x00D9D9D9, 0x00D8D8D8, 0x00D7D7D7, 0x00D5D5D5, 0x00D4D4D4, 0x00BCBCBC, 0x00E2E2E2};

rect_t rects[2];

int left_button_held_down;
int right_button_held_down;

int moving = 0;
window_t * moving_window;

#define DEBUG_GUI 0

int is_moving() {
    return moving;
}
void print_windows_depth() {
#if DEBUG_GUI
    int size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);

    for(int i = 0; i < size; i++) {
        window_t * curr_w = windows_array[i];
        qemu_printf("* %s, depth: %d\n", curr_w->name, curr_w->depth);
        qemu_printf("under_windows: [");
        foreach(t, curr_w->under_windows) {
            window_t * w = t->val;
            qemu_printf("%s ", w->name);
        }
        qemu_printf("]\n");
        qemu_printf("above_windows: [");
        foreach(t, curr_w->above_windows) {
            window_t * w = t->val;
            qemu_printf("%s ", w->name);
        }
        qemu_printf("]\n\n");
    }
#endif
}

window_t * get_focus_window() {
    return focus_w;
}

/*
 * The window needs to receive user inputs like mouse movement, keyboard keypress, and others.
 * */
void window_message_handler(winmsg_t * msg) {
    char key_pressed;
    window_t * w = msg->window;
    // Translate the cursor_x and cursor_y to base on the window's coord, not the screen's coord
    point_t p = get_canonical_coordinates(w);
    int cursor_x = msg->cursor_x - p.x;
    int cursor_y = msg->cursor_y - p.y;
    //qemu_printf("%s(d = %d): ", w->name, w->depth);
    rect_t r = rect_create(0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT);
    switch(msg->msg_type) {
        case WINMSG_MOUSE:
            if(msg->sub_type == WINMSG_MOUSE_MOVE) {
                //qemu_printf("mouse move\n");
                if(left_button_held_down) {
                    // Title bar
                    r.x = TITLE_BAR_HEIGHT * 2;
                    r.width = w->width - r.x;
                    if(is_point_in_rect(cursor_x, cursor_y, &r) || moving) {
                        if(moving) {
                            qemu_printf("%s is moving\n", moving_window->name);
                            move_window(moving_window, moving_window->x + msg->change_x, moving_window->y + msg->change_y);
                        }
                        else {
                            qemu_printf("%s is moving\n", w->name);
                            move_window(w, w->x + msg->change_x, w->y + msg->change_y);
                        }

                    }
                }
            }

            if(msg->sub_type == WINMSG_MOUSE_RIGHT_BUTTONUP) {
                right_button_held_down = 0;
            }

            if(msg->sub_type == WINMSG_MOUSE_RIGHT_BUTTONDOWN) {
                right_button_held_down = 1;
                //qemu_printf("right button held down\n");
            }

            if(msg->sub_type == WINMSG_MOUSE_LEFT_BUTTONUP) {
                qemu_printf("left button up\n");
                left_button_held_down = 0;
                if(moving) {
                    moving = 0;
                    moving_window = NULL;
                }
                if((w->type == WINDOW_NORMAL || w->type == WINDOW_ALERT) && strcmp(w->name, "desktop_bar") != 0){
                    // Close button
                    if(w->type == WINDOW_NORMAL || w->type == WINDOW_ALERT) {
                        if(is_point_in_rect(cursor_x, cursor_y, &r)) {
                            close_window(msg->window);
                        }
                    }
                    // Minimize button
                    r.x = TITLE_BAR_HEIGHT;
                    if(is_point_in_rect(cursor_x, cursor_y, &r)) {
                        minimize_window(msg->window);
                    }
                }
                else if(w->type == WINDOW_CONTROL) {
                    // Call button handler
                    if(strcmp("window_xp", w->name) == 0){
                        window_t * alertbox = alertbox_create(w->parent, 100, 100, "Alertbox", "A button is clicked!!!");
                        window_display(alertbox, NULL, 0);
                    }
                    else if(strcmp("alertbox_button", w->name) == 0){
                        // Get parent and close
                        close_window(w->parent);
                    }
                }
            }

            if(msg->sub_type == WINMSG_MOUSE_LEFT_BUTTONDOWN) {
                qemu_printf("left button down\n");
                left_button_held_down = 1;
                // Set window focus
                window_set_focus(w);
                //qemu_printf("left button held down\n");
            }

            break;
        case WINMSG_KEYBOARD:
            key_pressed = msg->key_pressed;
            //qemu_printf("%s receives a key press [%c]\n", w->name, key_pressed);
            switch(key_pressed) {
                case 'u':
                    move_window(w, p.x - 10, p.y - 10);
                    break;

                case 'i':
                    move_window(w, p.x + 10, p.y - 10);
                    break;

                case 'j':
                    move_window(w, p.x - 10, p.y + 10);
                    break;

                case 'k':
                    move_window(w, p.x + 10, p.y + 10);
                    break;

                case 'w':
                    move_window(w, p.x, p.y - 10);
                    break;

                case 's':
                    move_window(w, p.x, p.y + 10);
                    break;

                case 'a':
                    move_window(w, p.x - 10, p.y);
                    break;

                case 'd':
                    move_window(w, p.x + 10, p.y);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/*
 * Convenient function for creating an alertbox window
 */
window_t * alertbox_create(window_t * parent, int x, int y, char * title, char * text) {
    window_t * alertbox = window_create(parent, x, y, 200, 160, WINDOW_ALERT, "window_classic");
    window_add_headline(alertbox, "classic");
    window_add_close_button(alertbox);
    canvas_t canvas_alertbox = canvas_create(alertbox->width, alertbox->height, alertbox->frame_buffer);
    set_font_color(VESA_COLOR_BLACK + 1);
    draw_text(&canvas_alertbox, title, 1, 2);
    draw_text(&canvas_alertbox, text, 5, 2);

    // Add a OK button for the alertbox
    window_t * ok_button = window_create(alertbox, 30, 60, 110, 30, WINDOW_CONTROL, "alertbox_button");
    canvas_t canvas_button = canvas_create(ok_button->width, ok_button->height, ok_button->frame_buffer);
    draw_text(&canvas_button, "Close Button", 1, 1);

    return alertbox;
}


/*
 * Window create, given x,y, width,heigt, type, name, and parent window.
 */
window_t *  window_create(window_t * parent, int x, int y, int width, int height, int type, char * name) {
    gtreenode_t * subroot;
    // Allocate spae and initialize
    window_t * w = kcalloc(sizeof(window_t), 1);
    strcpy(w->name, name);
    w->under_windows = list_create();
    w->above_windows = list_create();
    // A window's x and y coordinates should be relative to its parent(when interpreting x/y value of a window)
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->type = type;
    w->parent = parent;
    w->is_minimized = 0;

    w->frame_buffer = kmalloc(width * height * 4);
    w->blended_framebuffer = kmalloc(width * height * 4);
    if(parent != NULL) {
        if(strcmp(name, "window_red") == 0)
            memsetdw(w->frame_buffer, 0xffff0000, width * height);
        else if(strcmp(name, "window_green") == 0)
            memsetdw(w->frame_buffer, 0xff00ff00, width * height);
        else if(strcmp(name, "window_blue") == 0)
            memsetdw(w->frame_buffer, 0xff0000ff, width * height);
        else if(strcmp(name, "window_black") == 0)
            memsetdw(w->frame_buffer, 0x00000001, width * height);
        else if(strcmp(name, "window_classic") == 0)
            memsetdw(w->frame_buffer, 0xffeeeeee, width * height);
        else if(strcmp(name, "window_xp") == 0)
            memsetdw(w->frame_buffer, 0xffF7F3F0, width * height);
        else if(strcmp(name, "alertbox_button") == 0)
            memsetdw(w->frame_buffer, 0xffF7F3F0, width * height);
        else if(strcmp(name, "desktop_bar") == 0)
            memsetdw(w->frame_buffer, 0xffDCE8EC , width * height);

    }
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
    w->depth = 0;

    if(strcmp(w->name, "desktop_bar") == 0)
        desktop_bar = w;
    focus_w = w;
    return w;
}

void window_set_focus(window_t * w) {
    if(focus_w == w) return;
    // Bring window to the front
    // Destroy the under and above list and re-calculate them
    if(w->type != WINDOW_SUPER) {
        //qemu_printf("Set focus start\n");
        print_windows_depth();
        int idx = 0;
        point_t p = get_canonical_coordinates(w);
        // Step 1: Find all rectangles that are involved (just the rectangle of w, since this is a minimize operation)
        rect_t w_rect = rect_create(p.x, p.y, w->width, w->height);
        // For all windows involved, recalculate the list: under_windows, above_windows
        int size = 0;
        tree2array(windows_tree, (void**)windows_array, &size);

        for(int i = 0; i < size; i++) {
            window_t * curr_w = windows_array[i];
            if(curr_w->is_minimized || curr_w == w) continue;
            p = get_canonical_coordinates(curr_w);
            rect_t curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
            if(is_rect_overlap(w_rect, curr_rect)) {
                // Remove w from curr_w's under_windows
                if((idx = list_contain(curr_w->under_windows, w)) != -1)
                    list_remove_by_index(curr_w->under_windows, idx);
                // Add w to curr w's above_windows
                if(list_contain(curr_w->above_windows, w) == -1) {
                    list_insert_back(curr_w->above_windows, w);
                    curr_w->depth = list_size(curr_w->above_windows);
                }
            }
        }

        list_destroy(w->above_windows);
        w->above_windows = list_create();
        add_under_windows(w);
        w->depth = list_size(w->above_windows);

        // Redraw
        p = get_canonical_coordinates(w);
        rect_t r = rect_create(p.x, p.y, w->width, w->height);
        window_display(w, NULL, 0);
        draw_mouse();
        video_memory_update(&r, 1);
    }
    focus_w = w;
    //qemu_printf("Set focus finish\n");
    print_windows_depth();
}

/*
 * Add round corners to window by setting some pixels transparent
 **/
void window_add_round_corner(window_t * w) {
    canvas_t canvas = canvas_create(w->width, w->height, w->frame_buffer);
    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 5 - i; j++) {
            set_pixel(&canvas, 0x0, j, i);
            set_pixel(&canvas, 0x0, w->width - 5 + i + j, i);
        }
    }
}

/*
 * Below is a set of functions that add components to a window by either 1) drawing on the frame buffer of the window, or adding child window(which has its own frame buffer and relative coord)
 * */

/*
 * Draw a headline for the window, given a headline string
 * window headline has height = TITLE_BAR_HEIGHT, hardcoded
 * */
void window_add_headline(window_t * w, char * headline) {
    // Draw a rectangle at the start of the window
    // It should have a headline area that gradually increases color in each row
    canvas_t canvas = canvas_create(w->width, w->height, w->frame_buffer);
    for(int i = 0; i < TITLE_BAR_HEIGHT; i++) {
        set_fill_color(title_bar_colors[i] | 0x88000000);
        draw_line(&canvas, 0, i, w->width, i);
    }
}

/*
 * Draw a close button for headline (size is 18*18)
 * */
void window_add_close_button(window_t * w) {
    set_fill_color(0x00ff0000);
    canvas_t canvas = canvas_create(w->width, w->height, w->frame_buffer);
    draw_rect(&canvas, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT);
}

/*
 * Draw a minimize button for headline
 * */
void window_add_minimize_button(window_t * w) {
    set_fill_color(0x00ffff00);
    canvas_t canvas = canvas_create(w->width, w->height, w->frame_buffer);
    draw_rect(&canvas, TITLE_BAR_HEIGHT, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT);
}


/*
 * Recursively find out what windows are under window w
 */
void add_under_windows(window_t * w) {
    recur_add_under_windows(w, windows_tree->root);
}

/*
 * Helper function for add_under_windows
 */
void recur_add_under_windows(window_t * w, gtreenode_t * subroot) {
    // Stop exploring any branches under w
    window_t * curr_w = subroot->value;
    if(curr_w == w) return;

    if(is_window_overlap(w, curr_w)) {
        // Only add to the list if they are not already there
        if(list_contain(w->under_windows, curr_w) == -1) {
            list_insert_back(w->under_windows, curr_w);
        }
        if(list_contain(curr_w->above_windows, w) == -1) {
            list_insert_back(curr_w->above_windows, w);
            curr_w->depth = list_size(curr_w->above_windows);
        }
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
void window_display(window_t * w, rect_t * rects, int size) {
    rect_region_t rect_reg;
    if(w->is_minimized || w->parent->is_minimized) return;
    set_fill_color(w->border_color);

    // If the frame buffer is not null, also draw the frame buffer
    if(w->frame_buffer) {
        rect_reg.region = w->blended_framebuffer;
        point_t p = get_canonical_coordinates(w);
        if(rects) {
            // Draw part(s) of the window
            for(int i = 0; i < size; i++) {
                rect_reg.r = rects[i];
                draw_rect_clip_pixels2(&canvas, &rect_reg, w->width, p.x, p.y);
            }
        }
        else {
            rect_reg.r.x = p.x;
            rect_reg.r.y = p.y;
            rect_reg.r.width = w->width;
            rect_reg.r.height = w->height;
            // Draw entire window
            draw_rect_pixels(&canvas, &rect_reg);
        }
        //repaint(rect_reg.r);
        display_recur(w->self);
    }
}

/*
 * Iterate through the whole tree and draw all window
 * This is slow! And ! It doesn't consider the case where windows overlap with each other
 * */
void display_all_window() {
    //window_display(get_super_window());
    // Get a list of windows
    int size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);
    // Bubble sort window list, by depth
    for(int i = 0; i < size - 1; i++) {
        for(int j = 0; j < size - 1; j++) {
            if(windows_array[j]->depth < windows_array[j + 1]->depth) {
                window_t * swap = windows_array[j];
                windows_array[j] = windows_array[j + 1];
                windows_array[j + 1] = swap;
            }
        }
    }

    for(int i = 0; i < size; i++) {
        window_display(windows_array[i], NULL, 0);
        video_memory_update(NULL, 0);
    }
    draw_mouse();
}

/*
 * Helper function for display_all_window
 */
void display_recur(gtreenode_t * t) {
    foreach(child, t->children) {
        gtreenode_t * node = child->val;
        window_t * w = node->value;
        // Only draw buttons and textbox using display_recur
        if(w->type == WINDOW_CONTROL)
            window_display(w, NULL, 0);
    }
}

/*
 * Getter for super window, it's esentially the desktop
 */
window_t * get_super_window() {
    return windows_tree->root->value;
}

window_t * get_desktop_bar() {
    return desktop_bar;
}


/*
 * The fill color is used when I drew window directly to the screen buffer, it's deprecated now.
 */
void set_window_fillcolor(window_t * w, uint32_t color) {
    w->fill_color = color;
}



/*
 * Move window to a start at (x,y)
 */
void move_window(window_t * w, int x, int y) {
    if(moving && moving_window != w) {
        return;
    }
    moving = 1;
    moving_window = w;
    if(w->type == WINDOW_SUPER || w->type == WINDOW_DESKTOP_BAR)
        return;
    rect_t dirty_rects[3];
    // Do some input check here, make sure the window can not be move outside of the screen
    if(x < 0)
        x = 0;
    if(y < desktop_bar->y + desktop_bar->height)
        y = desktop_bar->y + desktop_bar->y + desktop_bar->height;
    if(x + w->width >= canvas.width)
        x = canvas.width - w->width;
    if(y + w->height >= canvas.height)
        y = canvas.height - w->height;

    //qemu_printf("Move window from [%d, %d] to [%d, %d], w = %d, h = %d\n", w->x, w->y, x, y, w->width, w->height);
    rect_t curr_rect;
    int oldx = w->x;
    int oldy = w->y;
    int oldw = w->width;
    int oldh = w->height;
    int rect_size = 0;
    w->x = x;
    w->y = y;
    point_t p;

    rect_t rect1, rect2;

    // Step1: Find the two rectangles that need to be redrawn with something else
    // Calculate width and height of both rectangles
    rect1.width = abs(x - oldx);
    rect2.height = abs(y - oldy);
    rect1.height = oldh - rect2.height;
    rect2.width = oldw;
    // Calculate (x,y) of both rectangles
    if(x > oldx) {// OK
        rect1.x = oldx;
        rect2.x = oldx;
    }
    else {// OK
        // For x <= oldx
        rect1.x = x + oldw;
        rect2.x = oldx;
    }

    if(y > oldy) {// OK
        rect1.y = y;
        rect2.y = oldy;
    }
    else {// OK
        rect1.y = oldy;
        rect2.y = y + oldh;
    }
    if(rect1.width != 0 && rect1.height != 0) {
        dirty_rects[rect_size++] = rect1;
        //qemu_printf("%dth rect: [x: %d y: %d w: %d h: %d]\n", rect_size, rect1.x, rect1.y, rect1.width, rect1.height);
    }
    if(rect2.width != 0 && rect2.height != 0) {
        dirty_rects[rect_size++] = rect2;
        //qemu_printf("%dth rect: [x: %d y: %d w: %d h: %d]\n", rect_size, rect2.x, rect2.y, rect2.width, rect2.height);
    }

    // Step2: What are the windows and their intersections related to each rectangle
    int size = 0;
    int new_size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);
    window_dirty_region_t * dirty_regions = kcalloc(size, sizeof(window_dirty_region_t));
    for(int j = 0; j < size; j++) {
        window_t * curr_w = windows_array[j];
        if(curr_w->is_minimized || curr_w == w) continue;
        p = get_canonical_coordinates(curr_w);
        curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
        for(int i = 0; i < rect_size; i++) {
            if(is_rect_overlap(dirty_rects[i], curr_rect)) {
                // MAKE THIS A LIST OF INTERSECTION!
                dirty_regions[new_size].w = curr_w;
                dirty_regions[new_size].rects[dirty_regions[new_size].len] = find_rect_overlap(dirty_rects[i], curr_rect);
                dirty_regions[new_size].len++;
            }
        }
        new_size++;
    }


    // Step3: Sort windows by depth and draw window intersection
    for(int k = 0; k < new_size - 1; k++) {
        for(int p = 0; p < new_size - 1; p++) {
            if(dirty_regions[p].w->depth < dirty_regions[p + 1].w->depth) {
                window_dirty_region_t swap = dirty_regions[p];
                dirty_regions[p] = dirty_regions[p + 1];
                dirty_regions[p + 1] = swap;
            }
        }
    }

    // Before rendering, minimize the size of rectangles even further checking whether the region is covered by other windows(back face culling ?)



    for(int q = 0; q < new_size; q++) {
        if(dirty_regions[q].w == w) continue;
        window_display(dirty_regions[q].w, dirty_regions[q].rects, dirty_regions[q].len);
    }

    // Step4: Draw window w on new position
    window_display(w, NULL, 0);
    draw_mouse();

    curr_rect = rect_create(x, y, w->width, w->height);
    dirty_rects[rect_size++] = curr_rect;

    //qemu_printf("------\n");
    for(int i = 0; i < rect_size; i++) {
        //qemu_printf("dirty_rects[%d]: [%d,%d,%d,%d]", i, dirty_rects[i].x, dirty_rects[i].y, dirty_rects[i].width, dirty_rects[i].height);
    }
    //qemu_printf("------\n");
    video_memory_update(dirty_rects, rect_size);
    qemu_printf("Move finish\n");
    window_set_focus(w);
}

/*
 * Before creating/moving a window is done, the compositor should blend the window with what was previously
 * in the window rectangle.
 * The blended result of the window is written to w->blended_framebuffer, which will eventually be copied to the video memory
 * Note, when the window is moved to somewhere else, the blending should be calculated based on w->framebuffer instead of the blended ones
 * However, if we're to calculate window A over B, we should use B->blended_framebuffer instead of B->framebuffer
 * */
void blend_windows(window_t * w) {
    if(w->type == WINDOW_SUPER) {
        memcpy(w->blended_framebuffer, w->frame_buffer, w->width * w->height * 4);
        return;
    }
    rect_t curr_rect;
    point_t p = get_canonical_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int oldh = w->height;

    // Step 1: Find all rectangles that are involved (just the rectangle of w in this case)
    rect_t w_rect = rect_create(oldx, oldy, oldw, oldh);
    // Step2: Find all windows that intersect with the rectangle (for each window, determine the intersecting portion, which is also a rectangle)
    int size = 0;
    int new_size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);

    for(int i = 0; i < size; i++) {
        window_t * curr_w = windows_array[i];
        if(curr_w->is_minimized) continue;
        p = get_canonical_coordinates(curr_w);
        curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
        if(is_rect_overlap(w_rect, curr_rect)) {
            curr_w->intersection_rect = find_rect_overlap(w_rect, curr_rect);
            windows_array[new_size++] = curr_w;
        }
    }
    // Step3: Sort all window. For each window, find the portions that are not covered by other more shallow windows
    // Second part of step3 can greatly reduce latency for situation where multiple windows overlap each other, i may implement it later if necessary
    // But minimize window now looks pretty smooth already.

    // Bubble sort window list, by depth
    for(int i = 0; i < new_size - 1; i++) {
        for(int j = 0; j < new_size - 1; j++) {
            if(windows_array[j]->depth < windows_array[j + 1]->depth) {
                window_t * swap = windows_array[j];
                windows_array[j] = windows_array[j + 1];
                windows_array[j + 1] = swap;
            }
        }
    }

    // Blend w with each of the windows, from back to front
    //for(int i = 0; i < new_size; i++) {
    //    if(windows_array[i]->depth > w->depth)
    //    blend_window_rect(w, windows_array[i]);
    //}
    blend_window_rect(w, windows_array[0]);
}


void blend_window_rect(window_t * top_w, window_t * bottom_w) {
    uint32_t top_color, bottom_color, blended_color;
    point_t p;
    int idx;
    rect_t * bottom_rect = &bottom_w->intersection_rect;
    for(int i = bottom_rect->x; i < bottom_rect->x + bottom_rect->width; i++) {
        for(int j = bottom_rect->y; j < bottom_rect->y + bottom_rect->height; j++) {
            top_color = get_window_pixel(top_w, i, j);

            p = get_relative_coordinates(bottom_w, i, j);
            idx = bottom_w->width * p.y + p.x;
            bottom_color = bottom_w->blended_framebuffer[idx];

            blended_color = blend_colors(top_color, bottom_color);
            //blended_color = bottom_color;
            //blended_color = 0x000000ff;
            p = get_relative_coordinates(top_w, i, j);
            idx = top_w->width * p.y + p.x;
            top_w->blended_framebuffer[idx] = blended_color;
        }
    }
}

/*
 * Minimize window
 */
void minimize_window(window_t * w) {
    rect_t curr_rect;
    w->is_minimized = 1;
    point_t p = get_canonical_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int oldh = w->height;

    // Step 1: Find all rectangles that are involved (just the rectangle of w, since this is a minimize operation)
    rect_t w_rect = rect_create(oldx, oldy, oldw, oldh);
    // Step2: Find all windows that intersect with the rectangle (for each window, determine the intersecting portion, which is also a rectangle)
    int size = 0;
    int new_size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);

    for(int i = 0; i < size; i++) {
        window_t * curr_w = windows_array[i];
        if(curr_w->is_minimized) continue;
        p = get_canonical_coordinates(curr_w);
        curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
        if(is_rect_overlap(w_rect, curr_rect)) {
            curr_w->intersection_rect = find_rect_overlap(w_rect, curr_rect);
            windows_array[new_size++] = curr_w;
        }
    }
    // Step3: Sort all window. For each window, find the portions that are not covered by other more shallow windows
    // Second part of step3 can greatly reduce latency for situation where multiple windows overlap each other, i may implement it later if necessary
    // But minimize window now looks pretty smooth already.

    // Bubble sort window list, by depth
    for(int i = 0; i < new_size - 1; i++) {
        for(int j = 0; j < new_size - 1; j++) {
            if(windows_array[j]->depth < windows_array[j + 1]->depth) {
                window_t * swap = windows_array[j];
                windows_array[j] = windows_array[j + 1];
                windows_array[j + 1] = swap;
            }
        }
    }

    for(int i = 0; i < new_size; i++) {
        window_display(windows_array[i], &windows_array[i]->intersection_rect, 1);
    }
    draw_mouse();
    // Step4, draw each of the window(only the part that's not covered), from the deepest one to the shalloest one
    //display_all_window();
    rects[0] = w_rect;
    video_memory_update(rects, 1);
    //video_memory_update(NULL, 0);
}


/*
 * Different from minimize_window, close_window actually removes the window from the window tree,
 */
void close_window(window_t * w) {
    rect_t curr_rect;
    point_t p = get_canonical_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int oldh = w->height;
    // Remove the window from tree
    tree_remove(windows_tree, w->self);

    // Step 1: Find all rectangles that are involved (just the rectangle of w, since this is a minimize operation)
    rect_t w_rect = rect_create(oldx, oldy, oldw, oldh);
    // Step2: Find all windows that intersect with the rectangle (for each window, determine the intersecting portion, which is also a rectangle)
    int size = 0;
    int new_size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);

    for(int i = 0; i < size; i++) {
        window_t * curr_w = windows_array[i];
        if(curr_w->is_minimized) continue;
        p = get_canonical_coordinates(curr_w);
        curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
        if(is_rect_overlap(w_rect, curr_rect)) {
            curr_w->intersection_rect = find_rect_overlap(w_rect, curr_rect);
            windows_array[new_size++] = curr_w;
        }
    }
    // Step3: Sort all window. For each window, find the portions that are not covered by other more shallow windows
    // Bubble sort window list, by depth
    for(int i = 0; i < new_size - 1; i++) {
        for(int j = 0; j < new_size - 1; j++) {
            if(windows_array[j]->depth < windows_array[j + 1]->depth) {
                window_t * swap = windows_array[j];
                windows_array[j] = windows_array[j + 1];
                windows_array[j + 1] = swap;
            }
        }
    }

    for(int i = 0; i < new_size; i++) {
        window_display(windows_array[i], &windows_array[i]->intersection_rect, 1);
    }
    draw_mouse();
    // Step4, draw each of the window(only the part that's not covered), from the deepest one to the shalloest one
    rects[0] = w_rect;
    video_memory_update(rects, 1);
}

/*
 * Resize window, this function doesn't work now after I start to give frame buffer to every window, don't use it for now
 */
void resize_window(window_t * w, int new_width, int new_height) {

}

/*
 * Copy pixels in rect to dst
 * */
void copy_rect(uint32_t * dst, rect_t r) {
    for(int i = r.y; i < r.y + r.height; i++) {
        for(int j = r.x; j < r.x + r.width; j++) {
            *dst = intermediate_framebuffer[screen_width * i + j];
            dst++;
        }
    }
}

/*
 * Figure out which window has the point(x,y)
 * */
window_t * query_window_by_point(int x, int y) {
    // Very Important: Do not allocate any memory here, because this function is probably going to be called 99999 times per second, you don't want to call lots of mallocs in here
    // So, we will just assume that it's impossible that there are more than 20 windows stacked together and all contain one point
    window_t * possible_windows[20];

    // A list of windows that are standing on this pixel
    //list_t * possible_windows = list_create();
    int num = 0;
    find_possible_windows(x, y, windows_tree->root, possible_windows, &num);
    // Now, we get a list of possible windows, find which one is on top (whichever has the most children is the one on top!)
    int min_depth = 999999;
    window_t * top_window = NULL;

    window_t ** window_list = possible_windows;
    for(int i = 0; i < num; i++) {
        window_t * w = window_list[i];
        if(w->depth <= min_depth && w->is_minimized != 1) {
            min_depth = w->depth;
            top_window = w;
        }

    }
    return top_window;
}


/*
 * Helper function for repaint(), paint a pixel given its (x,y)
 * */
void paint_pixel(canvas_t * canvas, int x, int y) {
    window_t * top_window = query_window_by_point(x, y);
    if(top_window != NULL) {
        set_pixel(canvas, get_window_pixel(top_window, x, y), x, y);
    }
}

/*
 * Given a rectangle region, repaint the pixel it should have
 * */
void repaint(rect_t r) {
    rect_t curr_rect;
    point_t p;
    int size = 0;
    int new_size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);

    for(int i = 0; i < size; i++) {
        window_t * curr_w = windows_array[i];
        if(curr_w->is_minimized) continue;
        p = get_canonical_coordinates(curr_w);
        curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
        if(is_rect_overlap(r, curr_rect)) {
            curr_w->intersection_rect = find_rect_overlap(r, curr_rect);
            windows_array[new_size++] = curr_w;
        }
    }
    // Step3: Sort all window. For each window, find the portions that are not covered by other more shallow windows
    // Bubble sort window list, by depth
    for(int i = 0; i < new_size - 1; i++) {
        for(int j = 0; j < new_size - 1; j++) {
            if(windows_array[j]->depth < windows_array[j + 1]->depth) {
                window_t * swap = windows_array[j];
                windows_array[j] = windows_array[j + 1];
                windows_array[j + 1] = swap;
            }
        }
    }

    for(int i = 0; i < new_size; i++) {
        window_display(windows_array[i], &windows_array[i]->intersection_rect, 1);
    }

    // Step4, draw each of the window(only the part that's not covered), from the deepest one to the shalloest one
    //display_all_window();
    rects[0] = r;
    video_memory_update(rects, 1);

}

/*
 * A recursive function for finding windows that contains the point (x, y)
 * return value num: number of possible windows
 * */
void find_possible_windows(int x, int y, gtreenode_t * subroot, window_t ** possible_windows, int * num) {
    window_t * curr_w = subroot->value;

    if(!curr_w->is_minimized) {
        if(is_point_in_window(x, y, curr_w)) {
            possible_windows[*num] = curr_w;
            (*num)++;
        }
        foreach(child, subroot->children) {
            find_possible_windows(x, y, child->val, possible_windows, num);
        }
    }
}

/*
 * Given x,y in a global frame buffer
 * Find the corresponding pixel in the window
 * */
uint32_t get_window_pixel(window_t * w, int x, int y) {
    // Transform (x,y) so that it's relative to the windows's frame buffer
    point_t p = get_relative_coordinates(w, x, y);
    int idx = w->width * p.y + p.x;
    return w->frame_buffer[idx];
}


/*
 * Is a point in the rectangle ?
 */
int is_point_in_rect(int point_x, int point_y, rect_t * r) {
    return (point_x >= r->x && point_x < r->x + r->width) && (point_y >= r->y && point_y < r->y + r->height);
}

/*
 * Is a point in the window?
 */
int is_point_in_window(int x, int y, window_t * w) {
    rect_t r;
    point_t p = get_canonical_coordinates(w);
    r.x = p.x;
    r.y = p.y;
    r.width = w->width;
    r.height = w->height;
    return is_point_in_rect(x, y, &r);
}


/*
 * Getter for screen canvas
 */
canvas_t * get_screen_canvas() {
    return &canvas;
}

/*
 * Canonical coordinates is the coordinates relative to the whole screen
 * Relative coordinates is the coordiantes relative to its parent
 * This function convert canonical coords to relative
 */
point_t get_relative_coordinates(window_t * w, int x, int y) {
    point_t p = get_canonical_coordinates(w);
    p.x = x - p.x;
    p.y = y - p.y;
    return p;
}

/*
 * This function convert relative coords to canonical one
 */
point_t get_canonical_coordinates(window_t * w) {
    point_t p;
    p.x = w->x;
    p.y = w->y;
    // Calculate the canonical xy coords
    window_t * runner = w->parent;
    while(runner != NULL) {
        p.x += runner->x;
        p.y += runner->y;
        runner = runner->parent;
    }
    return p;
}

/*
 * Is two window overlap ?
 */
int is_window_overlap(window_t * w1, window_t * w2) {
    point_t p1 = get_canonical_coordinates(w1);
    point_t p2 = get_canonical_coordinates(w2);
    rect_t r1 = rect_create(p1.x, p1.y, w1->width, w1->height);
    rect_t r2 = rect_create(p2.x, p2.y, w2->width, w2->height);
    return is_rect_overlap(r1, r2);
}


/*
 * In my GUI system, instead of writing window's pixels to the video memory directly, I'll first write them to a intermediate buffer, and then sending the buffer to video memory
 * This avoids "refreshing the whole screen" every time window redraw.
 */
void video_memory_update(rect_t * rects, int len) {
    if(rects == NULL) {
        for(int i = 0; i < 1024 * 768; i++) {
            screen[i] = intermediate_framebuffer[i];
        }
    }
    else {
        for(int i = 0; i < len; i++) {
            rect_t rect = rects[i];
            for(int j = rect.y;  j < rect.y + rect.height; j++) {
                for(int k = rect.x; k < rect.x + rect.width; k++) {
                    int idx = get_pixel_idx(&canvas, k, j);
                    if(intermediate_framebuffer[idx] != 0x00000000) {
                        screen[idx] = intermediate_framebuffer[idx];
                    }
                }
            }
        }
    }
}

/*
 * Initialize compositor by getting info from vesa driver, creating screen cavans struct, and creating the super window and load desktop wallpaper
 */
void compositor_init() {
    intermediate_framebuffer = kmalloc(1024 * 768 * 4);
    // Get linear frame buffer from vesa driver
    screen = vesa_get_lfb();
    screen_width = vesa_get_resolution_x();
    screen_height = vesa_get_resolution_y();
    canvas = canvas_create(1024, 768, intermediate_framebuffer);


    // windows_list is only used when redrawing windows
    windows_list = list_create();
    // Initialize window tree to manage the windows to be created
    // Create a base window, this should be the parent of all windows(super window)
    windows_tree = tree_create();
    window_t * w = window_create(NULL, 0, 0, screen_width, screen_height, WINDOW_SUPER, "desktop");
    //window_add_headline(w, "");
    set_window_fillcolor(w, VESA_COLOR_GREEN);

#if 1
    bitmap_t * wallpaper = bitmap_create("/wallpaper.bmp");
    bitmap_to_framebuffer(wallpaper, w->frame_buffer);
#endif
    blend_windows(w);
#if 0
    memsetdw(w->frame_buffer, 0x00ff00ff,1024*768);
#endif

    // Default fill color is black
    fill_color =  (255 << 24) |(223 << 16) | (224 << 8) | 224;
}
