#include "types.h"
#include "vsfs.h"
#include "tty.h"
#include "stdlock.h"
#include "vm.h"
#include "trap.h"
#include "proc.h"
#include "panic.h"
#include "stdlib.h"

/* The process table lock must be acquired before accessing the ptable. */
slock_t ptable_lock;
/* The process table */
struct proc ptable[PTABLE_SIZE];
/* A pointer into the ptable of the running process */
struct proc* rproc;

void sched_init()
{
	/* Zero all of the processes (unused) */
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
		memset(ptable + x, 0, sizeof(struct proc));
	/* No process is running right now. */
	rproc = NULL;
	/* Initilize our process table lock */
	slock_init(&ptable_lock);
}

struct proc* get_proc_pid(int pid)
{
	int x;
        for(x = 0;x < PTABLE_SIZE;x++)
        {
		if(ptable[x].pid == pid)
			return ptable + x;
	}

	/* There is no process with that pid. */
	return NULL;
}

void sched(void)
{
	slock_acquire(&ptable_lock);
	scheduler();
}

void scheduler(void)
{
  slock_acquire(&ptable_lock);
  
  int i;
  int curr_index;
  for(i = 0; i < PTABLE_SIZE; i++){
    if(ptable + i == rproc){
      curr_index = i;
    }
  }  
  curr_index++;
  while(ptable[curr_index].state != PROC_RUNNABLE){
    if(curr_index == PTABLE_SIZE - 1){
      curr_index = 0;
    }
    curr_index++;
  } 
  rproc = &ptable[curr_index];	
  switch_uvm(rproc->pgdir);
}
