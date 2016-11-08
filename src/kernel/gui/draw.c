#include <draw.h>
#include <math.h>
#include <compositor.h>

// Want to draw something on screen ? write pixel to screen! Keep in mind though each pixel is 24 bit here(hard code 24-bit pixel for simplicity here).
uint32_t * screen = NULL;
// Default fill color
uint32_t fill_color;
int screen_width, screen_height;

/*
 * Given x and y, return the array index of that pixel in the linear frame buffer
 * */
int get_pixel_idx(int x, int y) {
    return screen_width * y + x;
}

void set_pixel(uint32_t val, int x, int y) {
    screen[get_pixel_idx(x, y)] = val;
}
void set_fill_color(uint32_t color) {
    fill_color = color;
}

void draw_rect(int x, int y, int width, int height) {
    for(int i = y; i < y + height; i++) {
        // Draw a horizontal line for each i
        for(int j = x; j < x + width; j++) {
            set_pixel(fill_color, j, i);
        }
    }
}

void draw_rect_pixels(rect_region_t * rect_region) {
    int k = 0;

    for(int i = rect_region->r.y + rect_region->r.height - 1; i >= rect_region->r.y; i--) {
        for(int j = rect_region->r.x; j < rect_region->r.x + rect_region->r.width; j++) {
            if(rect_region->region[k] != 0x0)
                set_pixel(rect_region->region[k], j, i);
            k++;
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2)
{
    int dx = x2-x1;
    int dy = y2-y1;
    int dxabs = abs(dx);
    int dyabs = abs(dy);
    int sdx = sign(dx);
    int sdy = sign(dy);
    int x = 0;
    int y = 0;
    int px = x1;
    int py = y1;

    set_pixel(fill_color, px, py);
    if (dxabs>=dyabs) {
        for(int i=0;i<dxabs;i++){
            y+=dyabs;
            if (y>=dxabs)
            {
                y-=dxabs;
                py+=sdy;
            }
            px+=sdx;
            set_pixel(fill_color, px, py);
        }
    }
    else {
        for(int i=0;i<dyabs;i++)
        {
            x+=dxabs;
            if (x>=dyabs)
            {
                x-=dyabs;
                px+=sdx;
            }
            py+=sdy;
            set_pixel(fill_color, px, py);
        }
    }
}

/*
 * Helper function for rect overlap detection
 * */
int is_line_overlap(int line1_x1, int line1_x2, int line2_x1, int line2_x2) {
    // These are the only 4 situations when a line can intersect with each other (partially intersect, inside another line segment, or contain another line segment)
    int ret1 = line1_x1 < line2_x1 && (line1_x1 >= line2_x1 && line1_x2 <= line2_x2);
    int ret2 = (line1_x1 >= line2_x1 && line1_x1 <= line2_x2) && (line1_x2 >= line2_x1 && line1_x2 <= line2_x2);
    int ret3 = (line1_x1 >= line2_x1 && line1_x1 <= line2_x2) && (line1_x2 > line2_x2);
    int ret4 = (line1_x1 < line2_x1) && (line1_x1 > line2_x2);
    return ret1 || ret2 || ret3 || ret4;
}

int is_rect_overlap(rect_t rect1, rect_t rect2) {
    // if both horizontal and vertical range of two rect overlaps, then they must overlap
    return is_line_overlap(rect1.x, rect1.x + rect1.width - 1, rect2.x, rect2.x + rect2.width - 1) && is_line_overlap(rect1.y, rect1.y + rect1.height - 1, rect2.y, rect2.y + rect2.height - 1);
}

rect_t rect_create(int x, int y, int width, int height) {
    rect_t r;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    return r;
}
