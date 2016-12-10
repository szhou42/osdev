#include <rtc.h>
#include <string.h>
#include <kheap.h>

// Global var, store current date and time
datetime_t current_datetime;

char * weekday_map[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
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

    uint8_t registerB = get_rtc_register(0x0B);

    // Convert BCD to binary values if necessary
    if (!(registerB & 0x04)) {
        current_datetime.second = (current_datetime.second & 0x0F) + ((current_datetime.second / 16) * 10);
        current_datetime.minute = (current_datetime.minute & 0x0F) + ((current_datetime.minute / 16) * 10);
        current_datetime.hour = ( (current_datetime.hour & 0x0F) + (((current_datetime.hour & 0x70) / 16) * 10) ) | (current_datetime.hour & 0x80);
        current_datetime.day = (current_datetime.day & 0x0F) + ((current_datetime.day / 16) * 10);
        current_datetime.month = (current_datetime.month & 0x0F) + ((current_datetime.month / 16) * 10);
        current_datetime.year = (current_datetime.year & 0x0F) + ((current_datetime.year / 16) * 10);
    }
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
 * Only support the format: "day hour:minute"
 * For example, "Sat 6:32"
 * */
char * datetime_to_str(datetime_t * dt) {
    char * ret = kcalloc(15, 1);
    char day[4];
    char hour[3];
    char min[3];

    memset(&day, 0x0, 4);
    memset(&hour, 0x0, 3);
    memset(&min, 0x0, 3);

    const char * weekday = weekday_map[get_weekday_from_date(dt)];
    strcpy(day, weekday);
    itoa(hour, dt->hour, 10);
    itoa(min, dt->minute, 10);

    strcpy(ret, day);
    strcat(ret, " ");
    strcat(ret, hour);
    strcat(ret, ":");
    strcat(ret, min);
    strcat(ret, " PM");
    return ret;
}

char * get_current_datetime_str() {
    return datetime_to_str(&current_datetime);
}

/*
 * Given a date, calculate it's weekday, using the algorithm described here: http://blog.artofmemory.com/how-to-calculate-the-day-of-the-week-4203.html
 * */
int get_weekday_from_date(datetime_t * dt) {
    char month_code_array[] = {0x0,0x3, 0x3, 0x6, 0x1, 0x4, 0x6, 0x2, 0x5, 0x0, 0x3, 0x5};
    char century_code_array[] = {0x4, 0x2, 0x0, 0x6, 0x4, 0x2, 0x0};    // Starting from 18 century

    // Simple fix...
    dt->century = 21;

    // Calculate year code
    int year_code = (dt->year + (dt->year / 4)) % 7;
    int month_code = month_code_array[dt->month - 1];
    int century_code = century_code_array[dt->century - 1 - 17];
    int leap_year_code = is_leap_year(dt->year, dt->month);

    int ret = (year_code + month_code + century_code + dt->day - leap_year_code) % 7;
    return ret;
}

int is_leap_year(int year, int month) {
    if(year % 4 == 0 && (month == 1 || month == 2)) return 1;
    return 0;
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
