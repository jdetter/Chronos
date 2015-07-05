#ifndef _KTIME_H_
#define _KTIME_H_

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
int ktime_seconds(void);

/**
 * Parse an rtc structure with the current system time parameters.
 */
void ktime_rtc(struct rtc_t* dst);

#endif
