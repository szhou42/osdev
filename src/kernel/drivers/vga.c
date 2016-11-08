#include <system.h>
#include <string.h>
#include <vga.h>

int curr_x = 0, curr_y = 0;
uint8_t curr_color = DEFAULT_COLOR;

/*
 * Initialize
 * */
void video_init() {
    clear();
}


void print_string(char * s) {
    while(*s != '\0') {
        print_char(*s);
        s++;
    }
}
/* Print a char to screen, under current x,y position
 * scroll if necessary
 * For now, we only care about normal character, tab, and newline character
 * */
void print_char(char c) {
    scroll();
    if(c == '\n') {
        curr_x = 0;
        curr_y++;
    }
    else if(c == '\t') {
        int i;
        for(i = 0; i < 4; i++)
            print_char(' ');
    }
    else if(c >= ' ') {
        PIXEL(curr_x, curr_y) = PAINT(c, curr_color);
        curr_x++;
        if(curr_x == 80) {
            curr_x = 0;
            curr_y++;
        }
    }
    update_cursor();
}

/*
 * Scroll down by one line, copy the line 1 to line 0, line 2 to line 1...... and delete the last line
 * */
void scroll() {
    // Move up
    void * start = (void*)SCREEN + 1 * WIDTH * 2;
    uint32_t size = curr_y * WIDTH * 2;
    if(curr_y < 25)
        return;
    memcpy(SCREEN, start, size);
    // Delete
    start = (void*)SCREEN + size;
    memsetw(start, PAINT(0x20, DEFAULT_COLOR), WIDTH);
    curr_y--;
}

/*
 * Update cursor
 * */
void update_cursor() {

    unsigned curr_pos = curr_y * WIDTH + curr_x;

    outportb(0x3D4, 14);
    outportb(0x3D5, curr_pos >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, curr_pos);
}


/*
 * Set current foreground color
 * */
void set_curr_color(uint8_t color) {
   curr_color = color & 0x0F;
}
/*
 * Get current foreground color
 * */
uint8_t get_curr_color() {
     return curr_color;
}

/*
 * Clear the screen, simply put space char on video memory
 * */
void clear() {
    memsetw(SCREEN, PAINT(0x20, DEFAULT_COLOR), WIDTH * HEIGHT / 2);
}

