#include <timer.h>
#include <isr.h>
#include <printf.h>
#include <compositor.h>
#include <draw.h>
#include <rtc.h>
#include <font.h>

// Number of ticks since system booted
uint32_t jiffies = 0;
uint16_t hz = 0;
// Functions that want to be woke up
list_t * wakeup_list;
/*
 * Init timer by register irq
 * */
void timer_init() {
    // IRQ0 will fire 100 times per second
    set_frequency(100);
    register_interrupt_handler(32, timer_handler);
    wakeup_list = list_create();
}

/*
 * Simulate sleep() in C, bad implementation, can use it for debug maybe ?
 * */
void sleep(int sec) {
    uint32_t end = jiffies + sec * hz;
    while(jiffies < end);
}

/*
 * Originally, the frequency is 18 hz, u can change it to whatever you want
 * */
void set_frequency(uint16_t h) {
    hz = h;
    uint16_t divisor = INPUT_CLOCK_FREQUENCY / h;
    // Init, Square Wave Mode, non-BCD, first transfer LSB then MSB
    outportb(TIMER_COMMAND, TIMER_ICW);
    outportb(TIMER_DATA, divisor & 0xFF);
    outportb(TIMER_DATA, (divisor >> 8) & 0xFF);

}

/*
 * Call some function every x seconds
 * */
void register_wakeup_call(wakeup_callback func, double sec) {
    uint32_t jiffy = jiffies + sec * hz;
    // Save the function sec, and jiffy to a list, when timer hits that func's jiffy it will call the func, and update next jiffies
    wakeup_info_t * w = kmalloc(sizeof(wakeup_info_t));
    w->func = func;
    w->sec = sec;
    w->jiffies = jiffy;
    list_insert_front(wakeup_list, w);
}

/*
 * timer irq handler
 * */
void timer_handler(register_t * reg) {
#if DEBUG_MULTITASK
    qemu_printf("Timer handler triggered...\n");
#endif
    jiffies++;
    memcpy(&saved_context, reg, sizeof(register_t));
    foreach(t, wakeup_list) {
         wakeup_info_t * w = t->val;
         w->func();
    }
    /*
    if(jiffies % 1080 == 0) {
        window_t * w = get_desktop_bar();
        canvas_t canvas_bar = canvas_create(w->width, w->height, w->frame_buffer);
        set_font_color(VESA_COLOR_BLACK+1);
        draw_text(&canvas_bar, get_current_datetime_str(), 1, 115);
    }
    */
}
