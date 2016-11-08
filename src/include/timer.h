#ifndef TIMER_H
#define TIMER_H
#include <isr.h>
#include <list.h>
#include <kheap.h>
#include <process.h>

#define INPUT_CLOCK_FREQUENCY 1193180
#define TIMER_COMMAND 0x43
#define TIMER_DATA 0x40
#define TIMER_ICW 0x36

extern uint32_t jiffies;
extern uint16_t hz;
typedef void (*wakeup_callback) ();

typedef struct wakeup_info {
    wakeup_callback func;
    double sec;
    uint32_t jiffies;
} wakeup_info_t;

void timer_init();
void sleep(int sec);
void set_frequency(uint16_t hz);
void register_wakeup_call(wakeup_callback func, double sec);
void timer_handler(register_t * reg);

#endif
