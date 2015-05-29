#include "types.h"
#include "chronos.h"
#include "stdlock.h"
#include "vsfs.h"
#include "tty.h"
#include "proc.h"

extern slock_t ptable_lock; /* Process table lock */
extern struct proc* ptable; /* The process table */
extern struct proc* rproc; /* The currently running process */

int sys_fork(void)
{
	return 0;
}

int sys_wait(void)
{
	return 0;
}

int sys_exec(void)
{
	return 0;
}

void sys_exit(void)
{
	
}

int sys_open(void)
{
	return 0;
}

int sys_close(void)
{
	return 0;
}

int sys_read(void)
{
	return 0;
}

int sys_write(void)
{
	return 0;
}
