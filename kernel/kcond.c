#include "kern/types.h"
#include "stdlock.h"
#include "file.h"
#include "syscall.h"
#include "pipe.h"
#include "devman.h"
#include "tty.h"
#include "fsman.h"
#include "proc.h"

extern struct proc* rproc;

void cond_init(cond_t* c)
{
	c->next_signal = 0;
	c->current_signal = 0;
}

void cond_signal(cond_t* c)
{
	uint* esp_saved = rproc->sys_esp;
	rproc->sys_esp = (uint*)&c;
	sys_signal_cv();
	rproc->sys_esp = esp_saved;
}

void cond_wait_spin(cond_t* c, slock_t* lock)
{
	uint* esp_saved = rproc->sys_esp;
        rproc->sys_esp = (uint*)&c;
	sys_wait_s();
	rproc->sys_esp = esp_saved;
}

void cond_wait_ticket(cond_t* c, tlock_t* lock)
{
	uint* esp_saved = rproc->sys_esp;
        rproc->sys_esp = (uint*)&c;
	sys_wait_t();
	rproc->sys_esp = esp_saved;
}
