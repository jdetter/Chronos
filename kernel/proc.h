#ifndef _PROC_H_
#define _PROC_H_

/* The amount of processes in the ptable */
#define PTABLE_SIZE	50
#define MAX_PROC_NAME 	0x20
#define MAX_PATH_LEN	0x60

#define FD_TYPE_NULL 	0x00
#define FD_TYPE_STDIN 	0x01
#define FD_TYPE_STDOUT 	0x02
#define FD_TYPE_STDERR 	0x03
#define FD_TYPE_FILE 	0x04
#define FD_TYPE_DEVICE 	0x05
#define FD_TYPE_PIPE	0x06

#define FD_PIPE_MODE_NULL  0x00
#define FD_PIPE_MODE_WRITE 0x01
#define FD_PIPE_MODE_READ  0x02

struct file_descriptor
{
	uchar type;
	uint flags;
	int seek;
	inode i;
	struct IODriver* device;
	pipe_t pipe;
	uchar pipe_type;
};

#define MAX_FILES 0x20

/* States for processes */
#define PROC_UNUSED 	0x00 /* This process is free */
#define PROC_EMBRYO	0x01 /* Process is getting initilized */
#define PROC_READY	0x02 /* Process has yet to be run, but is runnable */
#define PROC_RUNNABLE 	0x03 /* Process has been ran before and is ready */
#define PROC_RUNNING 	0x04 /* This process is currently on the CPU */
#define PROC_BLOCKED 	0x05 /* This process is waiting on IO */
#define PROC_ZOMBIE 	0x06 /* Parent process has been killed */
#define PROC_KILLED 	0x07 /* Process is waiting for parent cleanup. */

/* Blocked reasons */
#define PROC_BLOCKED_NONE 0x00 /* The process is not blocked */
#define PROC_BLOCKED_WAIT 0x01 /* The process is waiting on a child */
#define PROC_BLOCKED_COND 0x02 /* The thread is waiting on a condition */

struct proc
{
	tty_t t; /* The tty this program is attached to. */
	uint pid; /* The id of the process */
	uint uid; /* The id of the user running the process */
	uint gid; /* The id of the group running the process */
	struct file_descriptor file_descriptors[MAX_FILES];
	
	uint state; /* The state of the process */
	
	/**
	 * Security note: The kernel should never try to access memory that
	 * is outside of:
	 *     ->   stack_end <= x < stack_start
	 *     ->   heap_end > x >= heap_start
	 *
	 * Accesses outside of these ranges could cause page faults or other
	 * types of faults.
	 */
	
	/* Stack start will be greater than stack end */
	uint stack_start; /* The high memory, start of the stack */
	uint stack_end; /* Size of the stack in bytes. */
	/* Heap end will be greater than heap start. */
	uint heap_start; /* The low memory, start of the heap */
	uint heap_end; /* Size of the heap in bytes. */

	uchar block_type; /* If blocked, what are you waiting for? */
	cond_t* b_condition; /* The condition we might be waiting on */
	uint b_condition_signal; /* The condition ticket number. */
	int b_pid; /* The pid we are waiting on. */

	uchar zombie; /* Whether or not the parent has been killed. */
	struct proc* parent; /* The process that spawned this process */
	char name[MAX_PROC_NAME];
	char cwd[MAX_PATH_LEN]; /* Current working directory */

	pgdir* pgdir; /* The page directory for the process */
	uchar* k_stack; /* A pointer to the kernel stack for this process. */
	struct task_segment* tss; /* The task segment for this process */
	struct trap_frame* tf; /* A pointer to the trap frame from the int. */	
	uint entry_point; /* The address of the first instruction */
	uint context; /* The address at the top of the saved stack */
};

/**
 * Allocate a new process
 */
struct proc* alloc_proc();

/**
 * Initilize all of the variables needed for scheduling. (lock not needed)
 */
void sched_init();

/**
 * Start a new process with the given tty.
 */
struct proc* spawn_tty(tty_t t);

/**
 * Load the binary in path into the address space of process p. Returns the
 * size of the binary in memory.
 */
uint load_binary(const char* path, struct proc* p);

/**
 * Returns a pointer with the process id pid. If there is no such process,
 * then NULL is returned. (lock not needed)
 */
struct proc* get_proc_pid(int pid);

/**
 * Surrender a scheduling round.
 */
void yield(void);

/**
 * The process table lock is already held, just enter the scheduler.
 */
void yield_withlock(void);

/**
 * Restore scheduling context. (lock not needed)
 */
void sched(void);

/**
 * The scheduler loop. The ptable lock must be held in order to enter
 * this loop. (lock required)
 */
void scheduler(void);

#endif
