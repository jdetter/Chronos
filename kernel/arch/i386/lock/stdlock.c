/**
 * Authors: Amber Arnold <alarnold2@wisc.edu> 
 *		John Detter <jdetter@wisc.edu>
 *
 * A standard lock library implementation.
 */

#include <stdlock.h>
#include "x86.h"

void slock_acquire(slock_t* lock)
{
	while(xchg(&lock->val, 1) == 1);
}


void tlock_acquire(tlock_t* lock)
{
	int turn;
	do
	{
		turn = fetch_and_add(&lock->next_ticket, 1);
	} while(turn != lock->currently_serving);
}
