#include "types.h"
#include "pipe.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdlock.h"


int pipe_write(void *src, uint sz, pipe_t pipe ){
	slock_acquire(&pipe->gaurd);
	int byteswritten = 0;
	while(byteswritten!=sz){
		while(pipe->full == 1){
			cond_wait_spin(&(pipe->empty), &(pipe->guard));
		}
		int bytes_avail = 0;
		int bytes_left = sz - byteswritten; 
		if(pipe->write => pipe->read){
			bytes_avail = PIPE_DATA - pipe->write;
		}
		else{
			bytes_avail = pipe->read - pipe->write;
		}
		if(bytes_avail>bytes_left){
			bytes_avail = bytes_left;
		}
		memmove(pipe->buffer+pipe->write, src+byteswritten, bytes_avail);
		pipe->write = (pipe->write + bytes_avail) % PIPE_DATA; 
		if(pipe->write == pipe->read){
			pipe->full = 1;
			cond_signal(&fill);
			
		}
		byteswritten += bytes_avail;
	}
	slock_release(&pipe->gaurd);
	return sz;
	
}

int pipe_read(void *dst, uint sz, pipe_t pipe){
	slock_acquire(&pipe->gaurd);
	int bytesread = 0;
	while(bytesread!=sz){
		while(pipe->full == 0 && pipe->write != pipe->read){
			cond_wait_spin(&(pipe->fill), &(pipe->guard));
		}
		int bytes_avail = 0;
		int bytes_left = sz - bytesread; 
		if(pipe->write => pipe->read){
			bytes_avail = PIPE_DATA - pipe->write;
		}
		else{
			bytes_avail = pipe->read - pipe->write;
		}
		if(bytes_avail>bytes_left){
			bytes_avail = bytes_left;
		}
		memmove(dst+bytesread, pipe->buffer+pipe->read,  bytes_avail);
		pipe->read = (pipe->read + bytes_avail) % PIPE_DATA; 
		pipe->full = 0;
		if(pipe->write == pipe->read){
			cond_signal(&empty);
			
		}
		bytesread+= bytes_avail;
	}
	slock_release(&pipe->gaurd);
	return sz;
}
