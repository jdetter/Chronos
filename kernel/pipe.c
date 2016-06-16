/**
 * Author: Amber Arnold <alarnold2@wisc.edu>
 *
 * Circular buffer (pipe) implementation. Used for process
 * communication
 */

#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "stdarg.h"
#include "stdlock.h"
#include "pipe.h"
#include "file.h"
#include "devman.h"

#include "fsman.h"
#include "tty.h"
#include "proc.h"

/* Pipe table */
struct pipe pipe_table[MAX_PIPES];
slock_t pipe_table_lock;

void pipe_init(void)
{
	slock_init(&pipe_table_lock);
	memset(pipe_table, 0, sizeof(struct pipe) * MAX_PIPES);
}

pipe_t pipe_alloc(void)
{
	slock_acquire(&pipe_table_lock);
	pipe_t p = NULL;
	int x;
	for(x = 0;x < MAX_PIPES;x++)
	{
		if(!pipe_table[x].allocated)
		{
			p = pipe_table + x;
			p->allocated = 1;
			p->faulted = 0;
			break;
		}
	}

	slock_release(&pipe_table_lock);

	return p;
}

void pipe_fault(pipe_t t)
{
	slock_acquire(&ptable_lock);
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_BLOCKED 
			&& ptable[x].block_type == PROC_BLOCKED_COND
			&& (ptable[x].b_condition == &t->empty
			|| ptable[x].b_condition == &t->fill))
		{
			ptable[x].b_condition = NULL;
			ptable[x].block_type = 0;
			ptable[x].state = PROC_RUNNABLE;
		}
	}
	slock_release(&ptable_lock);
}

void pipe_free(pipe_t p)
{
	slock_acquire(&pipe_table_lock);
	p->allocated = 0;
	p->faulted = 0;
	slock_release(&pipe_table_lock);
}

int pipe_write(void *src, size_t sz, pipe_t pipe ){
	if(pipe->faulted) return -1;
	slock_acquire(&pipe->guard);
	int byteswritten = 0;
	while(byteswritten!=sz){
		/* Check for full buffer and pipe fault */
		if(pipe->full && pipe->faulted) break;
		/* Never wait on a faulted pipe */
		while(pipe->full == 1 && !pipe->faulted){
			cond_wait_spin(&(pipe->empty), &(pipe->guard));
		}
		if(pipe->faulted) break;
		int bytes_avail = 0;
		int bytes_left = sz - byteswritten; 
		if(pipe->write >= pipe->read){
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
			cond_signal(&pipe->fill);
			
		}
		byteswritten += bytes_avail;
	}
	slock_release(&pipe->guard);
	return byteswritten;
	
}

int pipe_read(void *dst, size_t sz, pipe_t pipe){
	if(pipe->faulted) return -1;
	slock_acquire(&pipe->guard);
	int bytesread = 0;
	while(bytesread!=sz){
		/* If the pipe has faulted, and its empty, break */
		if(pipe->faulted && !pipe->full && pipe->write == pipe->read)
			break;
		/* Never wait on a faulted pipe! */
		while(pipe->full == 0 && pipe->write == pipe->read
				 && !pipe->faulted){
			cond_wait_spin(&(pipe->fill), &(pipe->guard));
		}
		int bytes_avail = 0;
		int bytes_left = sz - bytesread; 
		if(pipe->write >= pipe->read){
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
			cond_signal(&pipe->empty);
			
		}
		bytesread+= bytes_avail;
	}
	slock_release(&pipe->guard);
	return bytesread;
}
