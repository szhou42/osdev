#include <mouse.h>
#include <pic.h>
#include <vesa.h>
#include <compositor.h>
#include <draw.h>
#include <bitmap.h>
#include <serial.h>
#include <math.h>

int mouse_x;
int mouse_y;
int screen_width;
int screen_height;

// The rectangle that current mouse is going to be in, this would save that region's original pixels
rect_region_t next_mouse_region;

// Current mouse region, same thing as next mouse_region, except that this saves the mouse icon
rect_region_t current_mouse_region;

// Mouse icon
bitmap_t * cursor_icon;

// For updating cursor
rect_t rects[2];

// Mouse button state
uint8_t prev_button_state[3];
uint8_t curr_button_state[3];

void print_button_state() {
    //qemu_printf("prev state: %d %d %d\n", prev_button_state[0], prev_button_state[1], prev_button_state[2]);
    //qemu_printf("curr state: %d %d %d\n", curr_button_state[0], curr_button_state[1], curr_button_state[2]);
}
int left_button_down() {
    return !prev_button_state[0] && curr_button_state[0];
}

int right_button_down() {
    return !prev_button_state[2] && curr_button_state[2];
}

int left_button_up() {
    return prev_button_state[0] && !curr_button_state[0];
}

int right_button_up() {
    return prev_button_state[2] && !curr_button_state[2];
}

/*
 * Every time mouse event fires, mouse_handler will be called
 * The argument regs is not used in here
 * */
void mouse_handler(register_t * regs)
{
    qemu_printf("Mouse interrupt fired\n");
    static uint8_t mouse_cycle = 0;
    static char mouse_byte[3];
    winmsg_t msg;
    // Cursor width and height can change, depending on its position(like when the half of the cursor is outside of screen)
    int cursor_curr_width = CURSOR_WIDTH;
    int cursor_curr_height = CURSOR_HEIGHT;

    // Fill message
    msg.msg_type = WINMSG_MOUSE;
    msg.cursor_x = mouse_x;
    msg.cursor_y = mouse_y;

    switch(mouse_cycle) {
        case 0:
            mouse_byte[0] = mouse_read();
            // Inform the window message handler about mouse operation
            if(MOUSE_LEFT_BUTTON(mouse_byte[0])) {
                curr_button_state[0] = 1;
            }
            else {
                curr_button_state[0] = 0;
            }

            if(MOUSE_RIGHT_BUTTON(mouse_byte[0])) {
                curr_button_state[2] = 1;
            }
            else {
                curr_button_state[2] = 0;
            }
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = mouse_read();
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2]= mouse_read();
            // Position is not changed
            //if(mouse_byte[1] == 0 && mouse_byte[2] == 0)
            //    break;

            // Update mouse position
            // Transform delta values using some sort of log function
            mouse_x = mouse_x + (mouse_byte[1]);
            mouse_y = mouse_y - (mouse_byte[2]);

            // Adjust mouse position
            if(mouse_x < 0)
                mouse_x = 0;
            if(mouse_y < 0)
                mouse_y = 0;
            if(mouse_x > screen_width - 1)
                mouse_x = screen_width - 1;
            if(mouse_y > screen_height - 1)
                mouse_y = screen_height - 1;

            // Repaint previous mouse region
            repaint(next_mouse_region.r);
            rects[0] = next_mouse_region.r;

            // Save current mouse rect region
            next_mouse_region.r.x = mouse_x;
            next_mouse_region.r.y = mouse_y;

            // Adjust cursor size
            if(mouse_x + CURSOR_WIDTH > screen_width - 1) {
                cursor_curr_width = screen_width - mouse_x;
            }

            if(mouse_y + CURSOR_HEIGHT > screen_height - 1) {
                cursor_curr_height = screen_height - mouse_y;
            }

            next_mouse_region.r.width = cursor_curr_width;
            next_mouse_region.r.height = cursor_curr_height;
            current_mouse_region.r = next_mouse_region.r;
            memsetdw(next_mouse_region.region, 0x0000ff00, CURSOR_WIDTH * CURSOR_HEIGHT);

            // Repaint current mouse region
            repaint(next_mouse_region.r);

            // Actually draw the mouse in here
            draw_mouse();

            // Only update the two rectangle video memory
            rects[1] = current_mouse_region.r;
            video_memory_update(rects, 2);
            // May be send a mouse move message to windows in here, if needed.

            msg.sub_type = WINMSG_MOUSE_MOVE;
            msg.change_x = mouse_byte[1];
            msg.change_y = -mouse_byte[2];
            //qemu_printf("x change = %d y change = %d\n", msg.change_x, msg.change_y);
            mouse_cycle = 0;
            break;
    }
    if(mouse_cycle == 0) {
        if(left_button_down()) {
            msg.sub_type = WINMSG_MOUSE_LEFT_BUTTONDOWN;
            //qemu_printf("left button down\n");
            print_button_state();
        }
        if(left_button_up()) {
            msg.sub_type = WINMSG_MOUSE_LEFT_BUTTONUP;
            //qemu_printf("left button up\n");
            print_button_state();
        }
        if(right_button_down()) {
            msg.sub_type = WINMSG_MOUSE_RIGHT_BUTTONDOWN;
            //qemu_printf("right button down\n");
            print_button_state();
        }
        if(right_button_up()) {
            msg.sub_type = WINMSG_MOUSE_RIGHT_BUTTONUP;
            //qemu_printf("right button up\n");
            print_button_state();
        }
        msg.window = query_window_by_point(mouse_x, mouse_y);
        window_message_handler(&msg);
        // Previous state becomes current state
        memcpy(prev_button_state, curr_button_state, 3);
        // Current state becomes empty
        memset(curr_button_state, 0x00, 3);
    }
}

void mouse_write(uint8_t a_write) //unsigned char
{
    //Tell the mouse we are sending a command
    mouse_wait(1);
    outportb(0x64, 0xD4);
    mouse_wait(1);
    //Finally write
    outportb(0x60, a_write);
}

uint8_t mouse_read()
{
    mouse_wait(0);
    return inportb(0x60);
}

void draw_mouse() {
    set_fill_color(VESA_COLOR_BLUE);
    draw_rect_clip_pixels(get_screen_canvas(), &current_mouse_region, CURSOR_WIDTH);
#if 0
    if(is_moving()) {
        point_t p = get_mouse_position_before_move();
        qemu_printf("Drawing mouse at (%d, %d)\n", p.x, p.y);
        current_mouse_region.r.x = p.x;
        current_mouse_region.r.x = p.y;
        draw_rect_clip_pixels(get_screen_canvas(), &current_mouse_region, CURSOR_WIDTH);
    }
    else {
        draw_rect_clip_pixels(get_screen_canvas(), &current_mouse_region, CURSOR_WIDTH);
    }
#endif
}

void mouse_wait(uint8_t a_type) {
    uint32_t _time_out=100000; //unsigned int
    if(a_type==0) {
        while(_time_out--) {
            if((inportb(0x64) & 1)==1)
            {
                return;
            }
        }
        return;
    }
    else {
        while(_time_out--) {
            if((inportb(0x64) & 2)==0) {
                return;
            }
        }
        return;
    }
}

/*
 * Mouse initialization, register irq and other stuff
 * */
void mouse_init() {
    // Screen width and height
    screen_width = vesa_get_resolution_x();
    screen_height = vesa_get_resolution_y();

    // Initial position of mouse
    //mouse_x = screen_width / 2;
    //mouse_y = screen_height / 2;
    mouse_x = 20;
    mouse_y = 20;

    // Save initial mouse rect region
    next_mouse_region.r.x = mouse_x;
    next_mouse_region.r.y = mouse_y;
    next_mouse_region.r.width = CURSOR_WIDTH;
    next_mouse_region.r.height = CURSOR_HEIGHT;
    next_mouse_region.region = kmalloc(CURSOR_WIDTH * CURSOR_HEIGHT * 4);
    memsetdw(next_mouse_region.region, 0x0000ff00, CURSOR_WIDTH * CURSOR_HEIGHT);
    // Cursoe icon
    cursor_icon = bitmap_create("/cursor.bmp");
    current_mouse_region.r = next_mouse_region.r;
    bitmap_to_framebuffer2(cursor_icon, current_mouse_region.region);
    // Draw the mouse
    rects[0] = next_mouse_region.r;
    draw_mouse();
    video_memory_update(rects, 1);

    uint8_t _status;  //unsigned char
    //Enable the auxiliary mouse device
    mouse_wait(1);
    outportb(0x64, 0xA8);

    //Enable the interrupts
    mouse_wait(1);
    outportb(0x64, 0x20);
    mouse_wait(0);
    _status=(inportb(0x60) | 2);
    mouse_wait(1);
    outportb(0x64, 0x60);
    mouse_wait(1);
    outportb(0x60, _status);

    // Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read();  //Acknowledge

    // Enable the mouse
    mouse_write(0xF4);
    mouse_read();  //Acknowledge

    // Setup the mouse handler
    register_interrupt_handler(IRQ_BASE + 12, mouse_handler);
}
