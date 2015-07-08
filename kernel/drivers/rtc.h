#ifndef _RTC_H_
#define _RTC_H_

struct rtc_t
{
	uchar seconds;
	uchar minutes;
	uchar hours;
	uchar day;
	uchar month;
	uchar year;
};

/**
 * Get the rtc time right now.
 */
void rtc_update(struct rtc_t* dst);

#endif
