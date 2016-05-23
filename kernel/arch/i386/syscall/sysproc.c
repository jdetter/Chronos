#include <sys/select.h>
#include <string.h>

#include "proc.h"
#include "stdlock.h"
#include "syscall.h"
#include "ktime.h"
#include "drivers/rtc.h"

extern slock_t ptable_lock;
extern struct proc* rproc;

/* int gettimeofday(struct timeval* tv, struct timezone* tz) */
int sys_gettimeofday(void)
{
        struct timeval* tv;
        struct timezone* tz;
        /* timezone is not used by any OS ever. It is purely historical. */
        if(syscall_get_int((int*)&tv, 0)) return -1;
        if(tv && syscall_get_buffer_ptr((void**)&tv,
                                sizeof(struct timeval), 0)) return -1;
        /* Is timezone specified? */
        if(syscall_get_int((int*)&tz, 1)) return -1;
        if(tz && syscall_get_buffer_ptr((void**)&tz,
                                sizeof(struct timeval), 1)) return -1;

        int seconds = ktime_seconds();

        if(tv)
        {
                tv->tv_sec = seconds;
                tv->tv_usec = 0;
        }

        if(tz)
        {
                memset(tz, 0, sizeof(struct timezone));
        }

        return 0;
}

/* uint sleep(uint seconds) */
int sys_sleep(void)
{
        uint seconds;
        if(syscall_get_int((int*)&seconds, 0)) return -1;
        slock_acquire(&ptable_lock);
        rproc->block_type = PROC_BLOCKED_SLEEP;
        int end = seconds + ktime_seconds() + 1;
        rproc->sleep_time = end;
        rproc->state = PROC_BLOCKED;
        while(1)
        {
                yield_withlock();
                if(ktime_seconds() >= end) break;
                slock_acquire(&ptable_lock);
        }

        return 0;
}
