#ifndef _PIPE_H_
#define _PIPE_H_

#define PIPE_DATA 0x1000 /* 16K */

int pipe_write(void *src, uint sz, pipe_t pipe );
int pipe_read(void *dst, uint sz, pipe_t pipe)
struct pipe
{
	uchar free; /* 1 = the pipe is not in use, 0 = the pipe is in use. */
	char buffer[PIPE_DATA];
	slock_t guard;
	int read;
	int write;
	cond_t empty, fill; 
	uchar full; 
	
};

typedef struct pipe* pipe_t;



#endif
