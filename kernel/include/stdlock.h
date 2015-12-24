#ifndef _STDLOCK_H_
#define _STDLOCK_H_

struct cond
{
	int next_signal;
	int current_signal;
};
typedef struct cond cond_t;

struct slock
{
	int val;
};
typedef struct slock slock_t;

struct tlock
{
        int next_ticket;
	int currently_serving;
};
typedef struct tlock tlock_t;

/**
 * Initilize the spin lock.
 */
void slock_init(slock_t* lock);

/**
 * Acquire the spin lock.
 */
void slock_acquire(slock_t* lock);

/**
 * Release the spin lock.
 */    
void slock_release(slock_t* lock);

/**
 * Initilize the ticket lock.
 */
void tlock_init(tlock_t* lock);

/**
 * Acquire the ticket lock.
 */
void tlock_acquire(tlock_t* lock);

/**
 * Release the ticket lock.
 */
void tlock_release(tlock_t* lock);

/**
 * Initilize a condition variable (non-broadcast).
 */    
void cond_init(cond_t* c);

/**
 * Send a signal to the condition variable.
 */
void cond_signal(cond_t* c);

/**
 * Wait on the condition variable c and release the spin lock.
 */    
void cond_wait_spin(cond_t* c, slock_t* lock);

/**
 * Wait on the condition variable c and release the ticket lock.
 */
void cond_wait_ticket(cond_t* c, tlock_t* lock);

#endif
