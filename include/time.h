#ifndef _TIME_H_
#define _TIME_H_

struct timeval 
{
	time_t		tv_sec; /* Seconds */
	suseconds_t 	tv_usec; /* Microseconds */
};

struct timezone 
{
	int tz_minuteswest; /* Minutes west of Greenwich UTC*/
	int tz_dsttime; /* type of Daylight Savings Time correction */
};

struct tms
{
	clock_t tms_utime; /* user time */
	clock_t tms_stime; /* system time */
	clock_t tms_cutime; /* user time of children */
	clock_t tms_cstime; /* system time of children */
};

#endif
