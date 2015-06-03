#include "types.h"
#include "vsfs.h"
#include "tty.h"
#include "stdlock.h"
#include "proc.h"
#include "vm.h"
#include "trap.h"
#include "panic.h"
#include "stdlib.h"
#include "x86.h"
#include "syscall.h"

/* The process table lock must be acquired before accessing the ptable. */
slock_t ptable_lock;
/* The process table */
struct proc ptable[PTABLE_SIZE];
/* A pointer into the ptable of the running process */
struct proc* rproc;
/* The next available pid */
uint next_pid;

void proc_init()
{
	next_pid = 0;
	memset(ptable, 0, sizeof(struct proc) * PTABLE_SIZE);
	rproc = NULL;
	slock_acquire(&ptable_lock);
}

struct proc* alloc_proc()
{
	slock_acquire(&ptable_lock);
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_UNUSED)
		{
			ptable[x].state = PROC_EMBRYO;
			break;
		}
	}

	slock_release(&ptable_lock);

	if(x >= PTABLE_SIZE) return NULL;
	return ptable + x;
}

void spawn_tty(tty_t t)
{
	/* Try to get a new process */
	struct proc* p = alloc_proc();
	if(!p) return; /* Could we find an unused process? */

	/* Get the process table lock */
	slock_acquire(&ptable_lock);

	/* Setup the new process */
	p->t = t;
	p->pid = next_pid++;
	p->uid = 0; /* init is owned by root */
	p->gid = 0; /* group is also root */
	memset(p->file_descriptors, 0, 
		sizeof(struct file_descriptor) * MAX_FILES);
	p->stack_sz = 0;
	p->heap_sz = 0;
	p->block_type = PROC_BLOCKED_NONE;
	p->b_condition = NULL;
	p->b_pid = 0;
	p->parent = p;
	strncpy(p->name, "init", MAX_PROC_NAME);
	strncpy(p->cwd, "/", MAX_PATH_LEN);
	p->pgdir = NULL;
	p->k_stack = (uchar*)(palloc() + PGSIZE);
	p->tf = (struct trap_frame*)(p->k_stack - sizeof(struct trap_frame));

	slock_release(&ptable_lock);
	
	/* Finish up by calling exec */
	const char* argv[1];
	argv[0] = "/bin/init";
	sys_exec(argv[0], argv);
}

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
  switch_uvm(rproc);
}
