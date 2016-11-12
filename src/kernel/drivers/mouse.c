#include <mouse.h>
#include <pic.h>
#include <vesa.h>
#include <compositor.h>
#include <draw.h>
#include <bitmap.h>

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
    next_mouse_region.r.width = 32;
    next_mouse_region.r.height = 32;
    next_mouse_region.region = kmalloc(32*32);
    //copy_rect(next_mouse_region.region, next_mouse_region.r);
    memsetdw(next_mouse_region.region, 0x0000ff00, 32*32);
    // Cursoe icon
    cursor_icon = bitmap_create("/cursor.bmp");
    current_mouse_region.r = next_mouse_region.r;
    //current_mouse_region.region = (uint32_t*)cursor_icon->image_bytes;
    bitmap_to_framebuffer2(cursor_icon, current_mouse_region.region);
    // Draw the mouse
    draw_mouse();

    uint8_t _status;  //unsigned char
    //Enable the auxiliary mouse device
    outportb(0x64, 0xA8);

    //Enable the interrupts
    outportb(0x64, 0x20);
    _status=(inportb(0x60) | 2);
    outportb(0x64, 0x60);
    outportb(0x60, _status);

    // Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read();  //Acknowledge

    // Enable the mouse
    mouse_write(0xF4);
    mouse_read();  //Acknowledge

    // Setup the mouse handler
    register_interrupt_handler(IRQ_BASE+12, mouse_handler);
}


/*
 * Every time mouse event fires, mouse_handler will be called
 * The argument regs is not used in here
 * */
void mouse_handler(register_t * regs)
{
    static uint8_t mouse_cycle = 0;
    static char mouse_byte[3];
    switch(mouse_cycle)
    {
        winmsg_t msg;
        case 0:
            mouse_byte[0] = mouse_read();
            if(MOUSE_LEFT_BUTTON(mouse_byte[0])) {
                // Call window message handler
                msg.msg_type = WINMSG_MOUSE;
                msg.sub_type = WINMSG_MOUSE_LEFTCLICK;
                msg.cursor_x = mouse_x;
                msg.cursor_y = mouse_y;
                msg.window = query_window_by_point(mouse_x, mouse_y);
                window_message_handler(&msg);

                printf("Your mouse's left button was clicked\n");
                display_all_window();
            }
            else if(MOUSE_RIGHT_BUTTON(mouse_byte[0])) {
                printf("Your mouse's right button was clicked\n");
                msg.msg_type = WINMSG_MOUSE;
                msg.sub_type = WINMSG_MOUSE_RIGHTCLICK;
                msg.cursor_x = mouse_x;
                msg.cursor_y = mouse_y;
                msg.window = query_window_by_point(mouse_x, mouse_y);
                window_message_handler(&msg);

#if 0
                window_t * w = query_window_by_point(mouse_x, mouse_y);
                minimize_window(w);
#endif
#if 0
                window_t * w = query_window_by_point(mouse_x, mouse_y);
                move_window(w, 50, 50);
#endif
#if 0
                window_t * w = query_window_by_point(mouse_x, mouse_y);
                close_window(w);
#endif
#if 0
                window_t * w = query_window_by_point(mouse_x, mouse_y);
                resize_window(w, 250, 250);
#endif
            }
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = mouse_read();
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2]= mouse_read();
            mouse_x = mouse_x + mouse_byte[1];
            mouse_y = mouse_y - mouse_byte[2];
            if(mouse_x < 0)
                mouse_x = 0;
            if(mouse_y < 0)
                mouse_y = 0;
            if(mouse_x > screen_width)
                mouse_x = screen_width;
            if(mouse_y > screen_height)
                mouse_y = screen_height;

            // Repaint previous mouse region
            repaint(next_mouse_region.r);
            printf("I think the above line crash");
            // Save current mouse rect region
            next_mouse_region.r.x = mouse_x;
            next_mouse_region.r.y = mouse_y;
            next_mouse_region.r.width = 32;
            next_mouse_region.r.height = 32;
            current_mouse_region.r = next_mouse_region.r;
            //copy_rect(next_mouse_region.region, next_mouse_region.r);
            memsetdw(next_mouse_region.region, 0x0000ff00, 32*32);

            // Repaint current mouse region
            repaint(next_mouse_region.r);
            // Actually draw the mouse in here
            draw_mouse();

            printf("Your mouse is now in(%d, %d)\n", mouse_x, mouse_y);

            mouse_cycle=0;
            break;
    }
}

void mouse_write(uint8_t a_write) //unsigned char
{
    //Tell the mouse we are sending a command
    outportb(0x64, 0xD4);
    //Finally write
    outportb(0x60, a_write);
}

uint8_t mouse_read()
{
    return inportb(0x60);
}

void draw_mouse() {
    set_fill_color(VESA_COLOR_BLUE);
    draw_rect_pixels(get_screen_canvas(), &current_mouse_region);
}
