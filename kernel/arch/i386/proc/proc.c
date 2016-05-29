#include <string.h>

#include "drivers/rtc.h"
#include "ktime.h"
#include "stdlock.h"
#include "proc.h"

extern int k_ticks;
extern struct rtc_t k_time;

void proc_init()
{
        next_pid = 0;
        memset(ptable, 0, sizeof(struct proc) * PTABLE_SIZE);
        rproc = NULL;
        k_ticks = 0;
        memset(&k_time, 0, sizeof(struct rtc_t));
        slock_init(&ptable_lock);
}
