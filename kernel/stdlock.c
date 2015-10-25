/**
 * Author: Amber Arnold <alarnold2@wisc.edu> 
 *
 * A standard lock library implementation.
 */

#include "kern/types.h"
#include "stdlock.h"
#include "x86.h"

void slock_init(slock_t* lock)
{
	lock->val = 0;
}

void slock_acquire(slock_t* lock)
{
	while(xchg(&lock->val, 1) == 1);
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
	int turn;
	do
	{
		turn = fetch_and_add(&lock->next_ticket, 1);
	}while(turn != lock->currently_serving);
}

void tlock_release(tlock_t* lock)
{
	lock->currently_serving++;
}
