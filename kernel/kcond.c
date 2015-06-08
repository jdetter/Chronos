#include "types.h"
#include "stdlock.h"

void cond_init(cond_t* c)
{

}


/*The wait() call
is executed when a thread wishes to put itself to sleep; the signal() call
is executed when a thread has changed something in the program and
thus wants to wake a sleeping thread waiting on this condition.*/
void cond_signal(cond_t* c)
{
	
}

void cond_wait_spin(cond_t* c, slock_t* lock)
{

}

void cond_wait_ticket(cond_t* c, tlock_t* lock)
{

}
