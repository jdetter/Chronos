#ifndef _RTC_H_
#define _RTC_H_

/**
 * RTC values
 */
struct rtc_t
{
	ushort seconds;
	ushort minutes;
	ushort hours;
	ushort day;
	ushort month; 
	ushort year;
};

/**
 * Get the rtc time right now.
 */
void rtc_update(struct rtc_t* dst);

/**
 * Get a string representation of the rtc time.
 */
void rtc_str(char* dst, size_t sz, struct rtc_t* r);

#endif
