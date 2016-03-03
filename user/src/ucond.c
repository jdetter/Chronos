#include "types.h"
#include "stdlock.h"

void cond_init(cond_t* c)
{
	c->next_signal = 0;
	c->current_signal = 0;
}

void cond_signal(cond_t* c)
{
	signal(c);
}

void cond_wait_spin(cond_t* c, slock_t* lock)
{
	wait_s(c, lock);
}

void cond_wait_ticket(cond_t* c, tlock_t* lock)
{
	wait_t(c, lock);
}
