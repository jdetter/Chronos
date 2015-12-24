#ifndef _SEM_H_
#define _SEM_H_

#define NUM_SEMS 32

struct sem
{
	int valid;
	/* Whatever types you need here. */
};

/* Semaphore type */
typedef struct sem sem_t;

/**
 * Initilize any globals at boot.
 */
void sem_kinit(void);

/**
 * Create and initilize a new semaphore. Returns the new
 * semaphore. Returns NULL if there are no more semaphores
 * available.
 */
sem_t* sem_alloc(void);

/**
 * Free the given semaphore. If NULL is passed as s, then
 * this function returns gracefully.
 */
void sem_free(sem_t* s);

/**
 * Wait on a semaphore.
 */
void sem_wait(sem_t* s);

/**
 * Release the semaphore.
 */
void sem_post(sem_t* s);

#endif
