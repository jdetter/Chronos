#ifndef _PIPE_H_
#define _PIPE_H_

#define PIPE_DATA 0x1000 /* 16K */

struct pipe
{
	uchar free; /* 1 = the pipe is not in use, 0 = the pipe is in use. */
	char buffer[PIPE_DATA];
	slock_t guard;
	
};

typedef struct pipe* pipe_t;



#endif
