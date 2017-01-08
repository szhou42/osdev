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
 * In Ubuntu's window's title bar, each row's color increments, instead of having one color for the entire title bar
 */
uint32_t title_bar_colors[TITLE_BAR_HEIGHT] = {0xff59584f, 0xff5f5d53, 0xff58564e, 0xff57554d, 0xff56544c, 0xff55534b, \
    0xff54524a, 0xff525049, 0xff514f48, 0xff504e47, 0xff4e4c45, 0xff4e4c45, \
        0xff4c4a44, 0xff4b4943, 0xff4a4842, 0xff484741, 0xff46453f, 0xff45443f, \
        0xff44433e, 0xff43423d, 0xff42413c, 0xff403f3a, 0xff3f3e3a};

rect_t rects[2];

int left_button_held_down, right_button_held_down;

int moving = 0;
window_t * moving_window;
point_t last_mouse_position;

int close_highlight, minimize_highlight, maximize_highlight;

bitmap_t * normal_close_bmp, * normal_minimize_bmp, *normal_maximize_bmp;
bitmap_t * highlight_close_bmp, * highlight_minimize_bmp, *highlight_maximize_bmp;

rect_region_t close_region, minimize_region, maximize_region;
rect_region_t highlight_close_region, highlight_minimize_region, highlight_maximize_region;

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
    int point_at_close = 0, point_at_minimize = 0, point_at_maximize = 0;
    window_t * w = msg->window;
    // char key_pressed;
    // Translate the cursor_x and cursor_y to base on the window's coord, not the screen's coord
    point_t p = get_device_coordinates(w);
    int cursor_x = msg->cursor_x - p.x,  cursor_y = msg->cursor_y - p.y;
    rect_t r = rect_create(0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT);
    rect_t r2 = rect_create(0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT);
    rect_t r3 = rect_create(0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT);
    switch(msg->msg_type) {
        case WINMSG_MOUSE:
            if(msg->sub_type == WINMSG_MOUSE_MOVE) {
                if(left_button_held_down) {
                    // Title bar (anything besides the window body and close/minimize/maximize buttons)
                    r.x = 62;
                    r.width = w->width - r.x;
                    if(is_point_in_rect(cursor_x, cursor_y, &r) || moving) {
                        if(moving) {
                            move_window(moving_window, moving_window->x + msg->change_x, moving_window->y + msg->change_y);
                        }
                        else {
                            //qemu_printf("%s Starts moving, cursor at (%d, %d), (%d, %d)\n", w->name, cursor_x, cursor_y, msg->cursor_x, msg->cursor_y);
                            last_mouse_position.x = cursor_x;
                            last_mouse_position.y = cursor_y;
                            move_window(w, w->x + msg->change_x, w->y + msg->change_y);
                        }

                    }
                }
                else {
                    // If w is desktop bar, return
                    if(w->type != WINDOW_NORMAL) return;
                    //qemu_printf("mouse over [%s]\n", w->name);
                    // Set button highlights
                    // Close button
                    r.x = 7; r.height= 17;
                    r2.x = 26; r2.y = 3;
                    r3.x = 45; r3.y = 3;
                    rect_t temp_rect;
                    canvas_t canvas = canvas_create(w->width, w->height, w->blended_framebuffer);
                    if((point_at_close = is_point_in_rect(cursor_x, cursor_y, &r))) {
                        draw_rect_pixels(&canvas, &highlight_close_region);
                        get_device_rect(&temp_rect, &highlight_close_region.r, &p);
                        close_highlight = 1;
                    }
                    // Minimize button
                    if((point_at_minimize = is_point_in_rect(cursor_x, cursor_y, &r2))) {
                        draw_rect_pixels(&canvas, &highlight_minimize_region);
                        get_device_rect(&temp_rect, &highlight_minimize_region.r, &p);
                        minimize_highlight = 1;
                    }
                    if((point_at_maximize = is_point_in_rect(cursor_x, cursor_y, &r3))) {
                        draw_rect_pixels(&canvas, &highlight_maximize_region);
                        get_device_rect(&temp_rect, &highlight_maximize_region.r, &p);
                        maximize_highlight = 1;
                    }
                    if(point_at_close || point_at_minimize || point_at_maximize) {
                        window_display(w, &temp_rect, 1);
                        draw_mouse();
                        video_memory_update(&temp_rect, 1);
                    }
                    // Cancel highlight, if highlight == 1 && point is not in rect
                    if(close_highlight && !point_at_close) {
                        draw_rect_pixels(&canvas, &close_region);
                        get_device_rect(&temp_rect, &close_region.r, &p);
                        window_display(w, &temp_rect, 1);
                        draw_mouse();
                        video_memory_update(&temp_rect, 1);
                        close_highlight = 0;
                    }
                    if(minimize_highlight && !point_at_minimize) {
                        draw_rect_pixels(&canvas, &minimize_region);
                        get_device_rect(&temp_rect, &minimize_region.r, &p);
                        window_display(w, &temp_rect, 1);
                        draw_mouse();
                        video_memory_update(&temp_rect, 1);
                        minimize_highlight = 0;

                    }
                    if(maximize_highlight && !point_at_maximize) {
                        draw_rect_pixels(&canvas, &maximize_region);
                        get_device_rect(&temp_rect, &maximize_region.r, &p);
                        window_display(w, &temp_rect, 1);
                        draw_mouse();
                        video_memory_update(&temp_rect, 1);
                        maximize_highlight = 0;
                    }
                }
            }

            if(msg->sub_type == WINMSG_MOUSE_RIGHT_BUTTONUP) {
                right_button_held_down = 0;
            }

            if(msg->sub_type == WINMSG_MOUSE_RIGHT_BUTTONDOWN) {
                right_button_held_down = 1;
            }

            if(msg->sub_type == WINMSG_MOUSE_LEFT_BUTTONUP) {
                left_button_held_down = 0;
                if(moving) {
                    moving = 0;
                    moving_window = NULL;
                }
                if((w->type == WINDOW_NORMAL || w->type == WINDOW_ALERT) && strcmp(w->name, "desktop_bar") != 0){
                    // Close button
                    if(w->type == WINDOW_NORMAL || w->type == WINDOW_ALERT) {
                        if(is_point_in_rect(cursor_x, cursor_y, &close_region.r)) {
                            close_window(msg->window);
                        }
                    }
                    // Minimize button
                    if(is_point_in_rect(cursor_x, cursor_y, &minimize_region.r)) {
                        minimize_window(msg->window);
                    }
                    if(is_point_in_rect(cursor_x, cursor_y, &maximize_region.r)) {
                        maximize_window(msg->window);
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
                left_button_held_down = 1;
                // Set window focus
                window_set_focus(w);
            }

            break;
        case WINMSG_KEYBOARD:
            //key_pressed = msg->key_pressed;
            // Process keyboard events here
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
    window_add_title_bar(alertbox);
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
            memsetdw(w->frame_buffer, 0xff300a24, width * height);
        else if(strcmp(name, "window_classic") == 0)
            memsetdw(w->frame_buffer, 0xffeeeeee, width * height);
        else if(strcmp(name, "window_xp") == 0)
            memsetdw(w->frame_buffer, 0xffF7F3F0, width * height);
        else if(strcmp(name, "alertbox_button") == 0)
            memsetdw(w->frame_buffer, 0xffF7F3F0, width * height);
        else if(strcmp(name, "desktop_bar") == 0)
            memsetdw(w->frame_buffer, 0xffDCE8EC , width * height);

    }
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
        int idx = 0;
        int size = 0;
        point_t p = get_device_coordinates(w);
        rect_t w_rect = rect_create(p.x, p.y, w->width, w->height);
        // For all windows involved, recalculate the list: under_windows, above_windows
        tree2array(windows_tree, (void**)windows_array, &size);

        for(int i = 0; i < size; i++) {
            window_t * curr_w = windows_array[i];
            if(curr_w->is_minimized || curr_w == w) continue;
            p = get_device_coordinates(curr_w);
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
        p = get_device_coordinates(w);
        rect_t r = rect_create(p.x, p.y, w->width, w->height);
        window_display(w, NULL, 0);
        draw_mouse();
        video_memory_update(&r, 1);
    }
    focus_w = w;
}


/*
 * Add round corners to window by setting some pixels transparent
 **/
void window_add_round_corner(window_t * w) {
    canvas_t canvas = canvas_create(w->width, w->height, w->frame_buffer);
    round_corner_effect(&canvas);
}

/*
 * Draw a title bar for the window
 * window title bar has height = TITLE_BAR_HEIGHT, hardcoded
 * */
void window_add_title_bar(window_t * w) {
    // Draw a rectangle at the start of the window
    // It should have a title bar area that gradually increases color in each row
    canvas_t canvas = canvas_create(w->width, w->height, w->frame_buffer);
    for(int i = 0; i < TITLE_BAR_HEIGHT; i++) {
        set_fill_color(title_bar_colors[i] | 0x88000000);
        draw_line(&canvas, 0, i, w->width, i);
    }
}

/*
 * Draw a close button for title bar (size is 18*18)
 * */
void window_add_close_button(window_t * w) {
    canvas_t close_btn_canvas = canvas_create(w->width, w->height, w->frame_buffer);
    // Load and draw close button
    draw_rect_pixels(&close_btn_canvas, &close_region);
}

/*
 * Draw a minimize button for title bar
 * */
void window_add_minimize_button(window_t * w) {
    canvas_t minimize_btn_canvas = canvas_create(w->width, w->height, w->frame_buffer);
    draw_rect_pixels(&minimize_btn_canvas, &minimize_region);
}

/*
 * Draw a maximize button for title bar
 * */
void window_add_maximize_button(window_t * w) {
    canvas_t maximize_btn_canvas = canvas_create(w->width, w->height, w->frame_buffer);
    draw_rect_pixels(&maximize_btn_canvas, &maximize_region);
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
        point_t p = get_device_coordinates(w);
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
    sort_windows_array(windows_array, size);
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

/*
 * Get a handle to desktop bar
 * */
window_t * get_desktop_bar() {
    return desktop_bar;
}

/*
 * Move window to (x,y)
 */
void move_window(window_t * w, int x, int y) {
    int oldx = w->x, oldy = w->y, oldw = w->width, oldh = w->height, rect_size = 0;
    rect_t dirty_rects[3];
    if(moving && moving_window != w) {
        return;
    }
    moving = 1;
    moving_window = w;
    if(w->type == WINDOW_SUPER || w->type == WINDOW_DESKTOP_BAR)
        return;
    // Do some input check here, make sure the window can not be move outside of the screen
    if(x < 0)
        x = 0;
    if(y < desktop_bar->y + desktop_bar->height)
        y = desktop_bar->y + desktop_bar->y + desktop_bar->height;
    if(x + w->width >= canvas.width)
        x = canvas.width - w->width;
    if(y + w->height >= canvas.height)
        y = canvas.height - w->height;

    rect_t curr_rect;
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
    }
    if(rect2.width != 0 && rect2.height != 0) {
        dirty_rects[rect_size++] = rect2;
    }

    // Step2: What are the windows and their intersections related to each rectangle
    int size = 0;
    int new_size = 0;
    tree2array(windows_tree, (void**)windows_array, &size);
    window_dirty_region_t * dirty_regions = kcalloc(size, sizeof(window_dirty_region_t));
    for(int j = 0; j < size; j++) {
        window_t * curr_w = windows_array[j];
        if(curr_w->is_minimized || curr_w == w) continue;
        p = get_device_coordinates(curr_w);
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
    blend_title_bar(w, 0);
    blend_title_bar(w, 1);
    window_display(w, NULL, 0);
    draw_mouse();

    curr_rect = rect_create(x, y, w->width, w->height);
    dirty_rects[rect_size++] = curr_rect;

    video_memory_update(dirty_rects, rect_size);
    window_set_focus(w);
}

/*
 * Blending entire window is expensive, instead,we only blend the top left and right corners (to implement round corners)
 * */
void blend_title_bar(window_t * w, int left) {
    if(w->type == WINDOW_SUPER) {
        memcpy(w->blended_framebuffer, w->frame_buffer, w->width * w->height * 4);
        return;
    }
    point_t p = get_device_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int new_size;
    // Step 1: Find all rectangles that are involved (just the rectangle of w in this case)
    rect_t w_rect;
    if(left)
        w_rect = rect_create(oldx, oldy, 10, TITLE_BAR_HEIGHT);
    else
        w_rect = rect_create(oldx + oldw - 10, oldy, 10, TITLE_BAR_HEIGHT);

    // Step2: Find all windows that intersect with the rectangle (for each window, determine the intersecting portion, which is also a rectangle)
    calculate_intersections(&w_rect, windows_array, &new_size);
    sort_windows_array(windows_array, new_size);
    // Blend w with each of the windows, from back to front
    for(int i = 0; i < new_size; i++) {
        if(windows_array[i] != w && windows_array[i]->depth > w->depth)
            blend_window_rect(w, windows_array[i]);
    }
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
    point_t p = get_device_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int oldh = w->height;

    rect_t w_rect = rect_create(oldx, oldy, oldw, oldh);
    int new_size;
    calculate_intersections(&w_rect, windows_array, &new_size);
    sort_windows_array(windows_array, new_size);

    // Blend w with each of the windows, from back to front
    for(int i = 0; i < new_size; i++) {
        if(windows_array[i]->depth > w->depth)
            blend_window_rect(w, windows_array[i]);
    }
}

/*
 * Blend top window with bottom window (only blend the region specified by bottom_w->intersection_rect)
 * */
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
            p = get_relative_coordinates(top_w, i, j);
            idx = top_w->width * p.y + p.x;
            top_w->blended_framebuffer[idx] = blended_color;
        }
    }
}

/*
 * Maximize window
 * */
void maximize_window(window_t * w) {
    qemu_printf("Hi, this is not implemented yet\n");
}

/*
 * Calculate the intersection between a rectangle and each of the windows
 * Write the intersection to window->intersection_rect
 * Also, return an array of windows, and size of the array
 * */
void calculate_intersections(rect_t * rect, window_t ** array, int * return_size) {
    int size = 0;
    int new_size = 0;
    point_t p;
    rect_t curr_rect;
    tree2array(windows_tree, (void**)array, &size);

    for(int i = 0; i < size; i++) {
        window_t * curr_w = array[i];
        if(curr_w->is_minimized) continue;
        p = get_device_coordinates(curr_w);
        curr_rect = rect_create(p.x, p.y, curr_w->width, curr_w->height);
        if(is_rect_overlap(*rect, curr_rect)) {
            curr_w->intersection_rect = find_rect_overlap(*rect, curr_rect);
            array[new_size++] = curr_w;
        }
    }
    *return_size = new_size;
}

/*
 * Sort an array of windows by depth
 * */
void sort_windows_array(window_t ** array, int size) {
    for(int i = 0; i < size - 1; i++) {
        for(int j = 0; j < size - 1; j++) {
            if(windows_array[j]->depth < windows_array[j + 1]->depth) {
                window_t * swap = windows_array[j];
                windows_array[j] = windows_array[j + 1];
                windows_array[j + 1] = swap;
            }
        }
    }
}

/*
 * Minimize window
 */
void minimize_window(window_t * w) {
    w->is_minimized = 1;
    point_t p = get_device_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int oldh = w->height;

    rect_t w_rect = rect_create(oldx, oldy, oldw, oldh);
    int new_size = 0;
    calculate_intersections(&w_rect, windows_array, &new_size);
    sort_windows_array(windows_array, new_size);

    for(int i = 0; i < new_size; i++) {
        window_display(windows_array[i], &windows_array[i]->intersection_rect, 1);
    }
    draw_mouse();
    // Step4, draw each of the window(only the part that's not covered), from the deepest one to the shalloest one
    rects[0] = w_rect;
    video_memory_update(rects, 1);
}


/*
 * Different from minimize_window, close_window actually removes the window from the window tree,
 */
void close_window(window_t * w) {
    point_t p = get_device_coordinates(w);
    int oldx = p.x;
    int oldy = p.y;
    int oldw = w->width;
    int oldh = w->height;
    // Remove the window from tree
    tree_remove(windows_tree, w->self);
    rect_t w_rect = rect_create(oldx, oldy, oldw, oldh);
    int new_size = 0;
    calculate_intersections(&w_rect, windows_array, &new_size);
    sort_windows_array(windows_array, new_size);
    for(int i = 0; i < new_size; i++) {
        window_display(windows_array[i], &windows_array[i]->intersection_rect, 1);
    }
    draw_mouse();
    // Step4, draw each of the window(only the part that's not covered), from the deepest one to the shalloest one
    rects[0] = w_rect;
    video_memory_update(rects, 1);
}

/*
 * Figure out which window has the point(x,y)
 * */
window_t * query_window_by_point(int x, int y) {
    // Very Important: Do not allocate any memory here, because this function is probably going to be called 99999 times per second, you don't want to call lots of mallocs in here
    // So, we will just assume that it's impossible that there are more than 20 windows stacked together and all contain one point
    window_t * possible_windows[20];

    // A list of windows that are standing on this pixel
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
 * Given a rectangle region, repaint the pixel it should have
 * */
void repaint(rect_t r) {
    int new_size = 0;
    calculate_intersections(&r, windows_array, &new_size);
    sort_windows_array(windows_array, new_size);
    for(int i = 0; i < new_size; i++) {
        window_display(windows_array[i], &windows_array[i]->intersection_rect, 1);
    }
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
 * Is a point in the window?
 */
int is_point_in_window(int x, int y, window_t * w) {
    rect_t r;
    point_t p = get_device_coordinates(w);
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
 * device coordinates is the coordinates relative to the whole screen
 * Relative coordinates is the coordiantes relative to its parent
 * This function convert device coords to relative
 */
point_t get_relative_coordinates(window_t * w, int x, int y) {
    point_t p = get_device_coordinates(w);
    p.x = x - p.x;
    p.y = y - p.y;
    return p;
}

/*
 * This function convert relative coordinates to device one
 */
point_t get_device_coordinates(window_t * w) {
    point_t p;
    p.x = w->x;
    p.y = w->y;
    // Calculate the device xy coords
    window_t * runner = w->parent;
    while(runner != NULL) {
        p.x += runner->x;
        p.y += runner->y;
        runner = runner->parent;
    }
    return p;
}

/*
 * Convert a rectangle from using relative coordinates to device coordinates
 * out_rect: output to this rectangle
 * relative_rect: The rectangle relative to window
 * p: Top left corner of the window
 * */
void get_device_rect(rect_t * out_rect, rect_t * relative_rect, point_t * p) {
    out_rect->x = relative_rect->x + p->x;
    out_rect->y = relative_rect->y +  p->y;
    out_rect->width = relative_rect->width;
    out_rect->height = relative_rect->height;
}


/*
 * Getter for mouse position before user initializing a move window operation
 * */
point_t get_mouse_position_before_move() {
    // First convert to device coordinates and then return
    point_t p = get_device_coordinates(moving_window);
    p.x = p.x + last_mouse_position.x;
    p.y = p.y + last_mouse_position.y;
    return p;
}
/*
 * Is two window overlap ?
 */
int is_window_overlap(window_t * w1, window_t * w2) {
    point_t p1 = get_device_coordinates(w1);
    point_t p2 = get_device_coordinates(w2);
    rect_t r1 = rect_create(p1.x, p1.y, w1->width, w1->height);
    rect_t r2 = rect_create(p2.x, p2.y, w2->width, w2->height);
    return is_rect_overlap(r1, r2);
}

/*
 * In my GUI system, instead of writing window's pixels to the video memory directly, I'll first write them to a intermediate buffer,
 * and then sending the buffer to video memory. This avoids screen flickering
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
                    screen[idx] = intermediate_framebuffer[idx];
                }
            }
        }
    }
}


void close_button_init() {
    normal_close_bmp = bitmap_create("/normal_close.bmp");
    highlight_close_bmp = bitmap_create("/red_highlight.bmp");
    close_region.r.x = 7; // (7)
    close_region.r.y = 3;
    close_region.r.width = normal_close_bmp->width;
    close_region.r.height = normal_close_bmp->height;
    close_region.region = (void*)normal_close_bmp->image_bytes;

    highlight_close_region.r.x = 7; // (7)
    highlight_close_region.r.y = 3;
    highlight_close_region.r.width = highlight_close_bmp->width;
    highlight_close_region.r.height = highlight_close_bmp->height;
    highlight_close_region.region = (void*)highlight_close_bmp->image_bytes;
}

void minimize_button_init() {
    normal_minimize_bmp = bitmap_create("/normal_minimize.bmp");
    highlight_minimize_bmp = bitmap_create("/minimize_highlight.bmp");
    minimize_region.r.x = 7 + 17 + 2; // (26)
    minimize_region.r.y = 3;
    minimize_region.r.width = normal_minimize_bmp->width;
    minimize_region.r.height = normal_minimize_bmp->height;
    minimize_region.region = (void*)normal_minimize_bmp->image_bytes;

    highlight_minimize_region.r.x = 7 + 17 + 2; // (26)
    highlight_minimize_region.r.y = 3;
    highlight_minimize_region.r.width = highlight_minimize_bmp->width;
    highlight_minimize_region.r.height = highlight_minimize_bmp->height;
    highlight_minimize_region.region = (void*)highlight_minimize_bmp->image_bytes;
}

void maximize_button_init() {
    normal_maximize_bmp = bitmap_create("/normal_maximize.bmp");
    highlight_maximize_bmp = bitmap_create("/maximize_highlight.bmp");
    maximize_region.r.x = 7 + 17 + 2 + 17 + 2; // (45)
    maximize_region.r.y = 3;
    maximize_region.r.width = normal_maximize_bmp->width;
    maximize_region.r.height = normal_maximize_bmp->height;
    maximize_region.region = (void*)normal_maximize_bmp->image_bytes;

    highlight_maximize_region.r.x = 7 + 17 + 2 + 17 + 2; // (45)
    highlight_maximize_region.r.y = 3;
    highlight_maximize_region.r.width = highlight_maximize_bmp->width;
    highlight_maximize_region.r.height = highlight_maximize_bmp->height;
    highlight_maximize_region.region = (void*)highlight_maximize_bmp->image_bytes;
}

/*
 * Initialize compositor by getting info from vesa driver, creating screen cavans struct,
 * and creating the desktop window with a wallpaper
 */
void compositor_init() {
    // Double buffering, avoid flickering
    intermediate_framebuffer = kmalloc(1024 * 768 * 4);
    // Get linear frame buffer from vesa driver
    screen = vesa_get_lfb();
    screen_width = vesa_get_resolution_x();
    screen_height = vesa_get_resolution_y();
    canvas = canvas_create(1024, 768, intermediate_framebuffer);
    // windows_list is only used when redrawing windows
    windows_list = list_create();
    // Initialize window tree to manage the windows to be created
    windows_tree = tree_create();
    // Create a base window, this should be the parent of all windows(super window)
    window_t * w = window_create(NULL, 0, 0, screen_width, screen_height, WINDOW_SUPER, "desktop");
    bitmap_t * wallpaper = bitmap_create("/wallpaper.bmp");
    bitmap_to_framebuffer(wallpaper, w->frame_buffer);
    blend_windows(w);
    // Initialize some data for buttons
    close_button_init();
    minimize_button_init();
    maximize_button_init();
}
