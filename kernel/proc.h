#ifndef _PROC_H_
#define _PROC_H_

/* The amount of processes in the ptable */
#define PTABLE_SIZE	50
#define MAX_PROC_NAME 	0x20

#define FD_TYPE_NULL 	0x00
#define FD_TYPE_STDIN 	0x01
#define FD_TYPE_STDOUT 	0x02
#define FD_TYPE_STDERR 	0x03
#define FD_TYPE_FILE 	0x04
#define FD_TYPE_DEVICE 	0x05

struct file_descriptor
{
	uchar type;
	int inode_pos;
	struct vsfs_inode inode;
};

#define MAX_FILES 0x20

/* States for processes */
#define PROC_UNUSED 	0x00
#define PROC_RUNNABLE 	0x01
#define PROC_RUNNING 	0x02
#define PROC_BLOCKED 	0x03
#define PROC_ZOMBIE 	0x04
#define PROC_KILLED 	0x05

/* Blocked reasons */
#define PROC_BLOCKED_NONE 0x00 /* The process is not blocked */
#define PROC_BLOCKED_WAIT 0x01 /* The process is waiting on a child */
#define PROC_BLOCKED_COND 0x02 /* The process is waiting on a condition */

struct proc
{
	tty_t t; /* The tty this program is attached to. */
	int pid; /* The id of the process */
	int uid; /* The id of the user running the process */
	int gid; /* The id of the group running the process */
	struct file_descriptor file_descriptors[MAX_FILES];
	
	uint state; /* The state of the process */
	uint stack_sz; /* Size of the stack in bytes. */
	uint heap_sz; /* Size of the heap in bytes. */

	uchar block_type; /* If blocked, what are you waiting for? */
	cond_t* b_condition; /* The condition we might be waiting on */
	int b_pid; /* The pid we are waiting on. */

	struct proc* parent; /* The process that spawned this process */
	struct vsfs_inode* cwd; /* The current working directory */
	char name[MAX_PROC_NAME];

	pgdir* pgdir; /* The page directory for the process */
	uchar* k_stack; /* A pointer to the kernel stack for this process. */
	struct trap_frame* tf; /* A pointer to the trap frame from the int. */	
};

/**
 * Initilize all of the variables needed for scheduling. (lock not needed)
 */
void sched_init();

/**
 * Returns a pointer with the process id pid. If there is no such process,
 * then NULL is returned. (lock not needed)
 */
struct proc* get_proc_pid(int pid);

/**
 * Acquire the ptable lock, then jump into the scheduler. (lock not needed)
 */
void sched(void);

/**
 * The scheduler loop. The ptable lock must be held in order to enter
 * this loop. (lock required)
 */
void scheduler(void);



#endif
