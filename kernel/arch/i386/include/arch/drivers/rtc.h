#ifndef _RTC_H_
#define _RTC_H_

#include <sys/types.h>

/**
 * RTC values
 */
struct rtc_t
{
	unsigned short seconds;
	unsigned short minutes;
	unsigned short hours;
	unsigned short day;
	unsigned short month; 
	unsigned short year;
};

/**
 * Get the rtc time right now.
 */
void rtc_update(struct rtc_t* dst);

/**
 * Get a string representation of the rtc time.
 */
void rtc_str(char* dst, size_t sz, struct rtc_t* r);

/**
 * Parse an rtc structure with the current system time parameters.
 */
void ktime_rtc(struct rtc_t* dst);

#endif
