#ifndef _PIPE_H_
#define _PIPE_H_

#define MAX_PIPES 32
#define PIPE_DATA 0x1000 /* 4K */

struct pipe
{
	int faulted; /* 0 = no error, 1 = one end of the pipe is closed. */
	int allocated; /* 0 = the pipe is not in use, 1 = the pipe is in use. */
	char buffer[PIPE_DATA];
	slock_t guard;
	int read;
	int write;
	cond_t empty, fill; 
	int full; 
	int read_ref; /* How many readers are there? */
	int write_ref; /* How many writers are there? */
};

typedef struct pipe* pipe_t;

void pipe_init(void);

/**
 * Allocate a new pipe from the pipe table.
 */
pipe_t pipe_alloc(void);

/**
 * Free a pipe that is no longer in use.
 */
void pipe_free(pipe_t p);

/**
 * Write to a pipe.
 */
int pipe_write(void *src, size_t sz, pipe_t pipe );

/**
 * Read from a pipe.
 */
int pipe_read(void *dst, size_t sz, pipe_t pipe);

#endif
