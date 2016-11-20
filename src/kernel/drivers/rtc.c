#include <rtc.h>
#include <string.h>

// Global var, store current date and time
datetime_t current_datetime;

/*
 * Check if rtc is updating time currently
 * */
int is_updating_rtc() {
    outportb(CMOS_ADDR, 0x0A);
    uint32_t status = inportb(CMOS_DATA);
    return (status & 0x80);
}

/*
 * Get the value of a specific rtc register
 * */
uint8_t get_rtc_register(int reg_num) {
    outportb(CMOS_ADDR, reg_num);
    return inportb(CMOS_DATA);
}

/*
 * Set the value of a specific rtc register
 * */
void set_rtc_register(uint16_t reg_num, uint8_t val) {
    outportb(CMOS_ADDR, reg_num);
    outportb(CMOS_DATA, val);
}

/*
 * Read current date and time from rtc, store in global var current_datetime
 * */
void rtc_read_datetime() {
    // Wait until rtc is not updating
    while(is_updating_rtc());

    current_datetime.second = get_rtc_register(0x00);
    current_datetime.minute = get_rtc_register(0x02);
    current_datetime.hour = get_rtc_register(0x04);
    current_datetime.day = get_rtc_register(0x07);
    current_datetime.month = get_rtc_register(0x08);
    current_datetime.year = get_rtc_register(0x09);
}

/*
 * Write a datetime struct to rtc
 * */
void rtc_write_datetime(datetime_t * dt) {
     // Wait until rtc is not updating
    while(is_updating_rtc());

    set_rtc_register(0x00, dt->second);
    set_rtc_register(0x02, dt->minute);
    set_rtc_register(0x04, dt->hour);
    set_rtc_register(0x07, dt->day);
    set_rtc_register(0x08, dt->month);
    set_rtc_register(0x09, dt->year);
}

/*
 * A convenient function that converts a datetime struct to string
 * */
char * datetime_to_str(datetime_t * dt) {
    return NULL;
}

/*
 * Initialize RTC
 * */
void rtc_init() {
/*
    current_datetime.century = 21;
    current_datetime.year = 16;
    current_datetime.month = 1;
    current_datetime.day = 1;
    current_datetime.hour = 0;
    current_datetime.minute = 0;
    current_datetime.second = 0;
    rtc_write_datetime(&current_datetime);
*/
    rtc_read_datetime();
}
