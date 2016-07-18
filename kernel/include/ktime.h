#ifndef _KTIME_H_
#define _KTIME_H_

#include <sys/time.h>

struct tms
{
    clock_t tms_utime; /* user time */
    clock_t tms_stime; /* system time */
    clock_t tms_cutime; /* user time of children */
    clock_t tms_cstime; /* system time of children */
};

/* Seconds in a normal year */
#define T_SECONDS_YEAR 31557600
/* Seconds in a leap year */
#define T_SECONDS_LYEAR 31622400

/**
 * Initilize the kernel time keeping mechanism.
 */
void ktime_init(void);

/**
 * Update the current system time from the CMOS.
 */
void ktime_update(void);

/**
 * Retrieve the kernel time in seconds.
 */
time_t ktime_seconds(void);

#endif
