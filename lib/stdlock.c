#include "types.h"
#include "stdlock.h"


void slock_init(slock_t* lock)
{
	lock->val = 0;
}

void slock_acquire(slock_t* lock)
{
	//lock->val = 1;
}

void slock_release(slock_t* lock)
{
	lock->val = 0;
}

void tlock_init(tlock_t* lock)
{
	lock->next_ticket = 0;
	lock->currently_serving = 0;
}

void tlock_acquire(tlock_t* lock)
{
	//lock->currently_serving = 1;
	//lock->next_ticket+=1;
}

void tlock_release(tlock_t* lock)
{
	lock->currently_serving = 0;
}
