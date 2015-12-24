#include <string.h>


#include "stdlock.h"
#include "sem.h"

/**
 * Table of all available semaphores (statically allocated).
 * NUM_SEMS is defined in sem.h.
 */
static sem_t sem_table[NUM_SEMS];

/** 
 * Lock for the semaphore table. You can only allocate
 * semaphores from the table if you hold this lock
 * (see sem_alloc).
 */
static slock_t sem_table_lock;

void sem_kinit(void)
{
	/* Zero the semaphore table */
	memset(sem_table, 0, sizeof(sem_t) * NUM_SEMS);
	/* Initilize the semaphore lock */
	slock_init(&sem_table_lock);
}

sem_t* sem_alloc(void)
{
	sem_t* result = NULL;
	/* Find a semaphore that isn't in use. */

	/* We are about to start searching the table */
	slock_acquire(&sem_table_lock);

	int i;
	for(i = 0;i < NUM_SEMS;i++)
	{
		/* Is this semaphore in use? */
		if(!sem_table[i].valid)
		{
			/* We found a free semaphore */
			result = sem_table + i;
			/* Mark this semaphore as valid */
			result->valid = 1;
			break;
		}
	}

	/* We're done searching through the table */
	slock_release(&sem_table_lock);

	/** TODO: Initilize the semaphore(result) here. */

	return result;

}

void sem_free(sem_t* s)
{
	if(!s) return;
	
	slock_acquire(&sem_table_lock);
	
	/* Is this pointer within our table? */
	if((char*)s >= (char*)sem_table &&
		(char*)s < (char*)(sem_table + NUM_SEMS))
	{
		/* Clear this semaphore. */
		memset(s, 0, sizeof(sem_t));
	}
	
	slock_release(&sem_table_lock);
}

void sem_wait(sem_t* s)
{

}

void sem_post(sem_t* s)
{

}
