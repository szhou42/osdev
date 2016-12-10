#ifndef RTC_H
#define RTC_H

#include <system.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

typedef struct datetime {
    uint8_t century;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
}datetime_t;

int is_updating_rtc();

uint8_t get_rtc_register(int reg_num);

void set_rtc_register(uint16_t reg_num, uint8_t val);

void rtc_read_datetime();

void rtc_write_datetime(datetime_t * dt);

char * datetime_to_str(datetime_t * dt);

char * get_current_datetime_str();

int get_weekday_from_date(datetime_t * dt);

int is_leap_year(int year, int month);

void rtc_init();

#endif
