/**
 * Author: John Detter <john@detter.com>
 *
 * Kernel time keeper.
 */

#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "ktime.h"
#include "drivers/rtc.h"
#include "stdlock.h"
#include "stdarg.h"
#include "x86.h"

slock_t rtc_lock;
struct rtc_t k_time;

void ktime_init(void)
{
        slock_init(&rtc_lock);
        memset(&k_time, 0, sizeof(struct rtc_t));
}

void ktime_rtc(struct rtc_t* dst)
{
        memmove(dst, &k_time, sizeof(struct rtc_t));
}

void ktime_update(void)
{
        slock_acquire(&rtc_lock);
        rtc_update(&k_time);
        slock_release(&rtc_lock);
}

time_t ktime_seconds(void)
{
	slock_acquire(&rtc_lock);
        /** 
         * Seconds between 00:00:00 January 1st, 1970 and 
         * 00:00:00 January 1st, 2000 
         */

        int epoc_year = 1970;
        int years = k_time.year - epoc_year; /* The last year is incomplete */
        /* 
         * How many years are between epoc_year and the previous leap year?
         */
        int leap_year_adjust = 2;
        /* Calulate number of leap years */
        int leap_years = (years + leap_year_adjust) / 4;
        /* Calculate number of normal years */
        int normal_years = years - leap_years;

        /* Is this year a leap year? */
        char is_leap = k_time.year % 4 == 0;
        uchar months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if(is_leap) months[1]++; /* Increment Feburary for leap years */
        int day_of_year = 0;
        int mon; /* Last month is incomplete */
        for(mon = 0;mon < k_time.month  - 1;mon++)
                day_of_year += months[mon];

        /* Add days this month (today is still incomplete) */
        day_of_year += k_time.day - 1;

        /* How many seconds today? */
        int seconds_today = (k_time.hours * 60 * 60)
                        + (k_time.minutes * 60)
                        + k_time.seconds;
        int days = leap_years * 366 + normal_years * 365 + day_of_year;
        int total = days * 24 * 60 * 60 + seconds_today;
	slock_release(&rtc_lock);

        return total;
}
