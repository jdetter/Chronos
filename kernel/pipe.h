#ifndef _PIPE_H_
#define _PIPE_H_

#define MAX_PIPES 32
#define PIPE_DATA 0x1000 /* 4K */

struct pipe
{
	uchar allocated; /* 0 = the pipe is not in use, 1 = the pipe is in use. */
	char buffer[PIPE_DATA];
	slock_t guard;
	int read;
	int write;
	cond_t empty, fill; 
	uchar full; 
	int references; /* How many file descriptors reference this pipe? */
};

typedef struct pipe* pipe_t;

void pipe_init(void);

/**
 * Allocate a new pipe from the pipe table.
 */
pipe_t alloc_pipe(void);

/**
 * Free a pipe that is no longer in use.
 */
void free_pipe(pipe_t p);

/**
 * Write to a pipe.
 */
int pipe_write(void *src, uint sz, pipe_t pipe );

/**
 * Read from a pipe.
 */
int pipe_read(void *dst, uint sz, pipe_t pipe);

#endif
