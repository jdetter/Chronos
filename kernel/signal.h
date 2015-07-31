#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#define MAX_SIGNAL 64 /* Size of the signal table */

                      /* SIG */
#define SIG_MAGIC 0x00000516

#define SIG_DEFAULT_GUARD 2 /* Default number of signal guard pages. */
#define SIG_DEFAULT_STACK 5 /* Default number of signal stack pages. */
/* Signal bounds checking */
#define SIG_FIRST 1  /* SIGHUP */
#define SIG_LAST  31 /* SIGSYS */

/**
 * Default signal actions.
 */
#define SIGDEFAULT_KPANIC 	0x00 /* Invalid signal */
#define SIGDEFAULT_TERM 	0x01 /* terminate the process */
#define SIGDEFAULT_CORE		0x02 /* terminate + core dump */
#define SIGDEFAULT_STOP		0x03 /* Stop the process */
#define SIGDEFAULT_CONT		0x04 /* Continue the process */
#define SIGDEFAULT_IGN		0x05 /* Ignore the signal */

struct signal_t
{
	uchar allocated; /* Whether or not this entry is allocated. */
	uchar type; /* Type of signal */
	uchar default_action; /* Default action */
	uchar catchable; /* Can this signal be caught? */
	uint handling; /* Are we handling this right now? */
	struct trap_frame handle_frame; /* Saved state information */

	/**
	 * If this signal is part of a queue, this is a pointer to the next
	 * signal in the queue.
	 */
	struct signal_t* next;
};

/**
 * Initilize signals
 */
void sig_init(void);

/**
 * Once we are done handling a signal, clean it up.
 */
int sig_cleanup(void);

/**
 * Clear all waiting signals for a process. Returns 0 on success.
 * Warning: ptable lock must be held.
 */
int sig_clear(struct proc* p);

/**
 * Send a signal to a process. Returns 0 on success.
 * Warning: ptable lock must be held.
 */
int sig_proc(struct proc* p, int sig);

/**
 * Handle a signal that is waiting.
 * Warning: ptable lock must be held.
 */
int sig_handle(void);

#endif
