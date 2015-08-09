#ifndef _PROC_H_
#define _PROC_H_

/* The amount of processes in the ptable */
#define PTABLE_SIZE	50
#define MAX_PROC_NAME 	0x20
#define MAX_PATH_LEN	0x60

#define FD_TYPE_NULL 	0x00
// #define FD_TYPE_STDIN 	0x01
// #define FD_TYPE_STDOUT 	0x02
// #define FD_TYPE_STDERR 	0x03
#define FD_TYPE_FILE 	0x04
#define FD_TYPE_DEVICE 	0x05
#define FD_TYPE_PIPE	0x06
#define FD_TYPE_PATH	0x07

#define FD_PIPE_MODE_NULL  0x00
#define FD_PIPE_MODE_WRITE 0x01
#define FD_PIPE_MODE_READ  0x02

struct file_descriptor
{
	uchar type;
//	int flags;
	int seek;
	inode i;
	struct DeviceDriver* device;
	pipe_t pipe;
	uchar pipe_type;
	char path[FILE_MAX_PATH];
};

#define MAX_FILES 0x20

/* States for processes */
#define PROC_UNUSED 	0x00 /* This process is free */
#define PROC_EMBRYO	0x01 /* Process is getting initilized */
#define PROC_READY	0x02 /* Process has yet to be run, but is runnable */
#define PROC_RUNNABLE 	0x03 /* Process has been ran before and is ready */
#define PROC_RUNNING 	0x04 /* This process is currently on the CPU */
#define PROC_BLOCKED 	0x05 /* This process is waiting on IO */
#define PROC_ZOMBIE 	0x06 /* Process called exit. (needs parent wait) */
#define PROC_KILLED 	0x07 /* Process recieved kill signal. */
#define PROC_STOPPED 	0x08 /* Process recieved stop signal. */

/* Blocked reasons */
#define PROC_BLOCKED_NONE 0x00 /* The process is not blocked */
#define PROC_BLOCKED_WAIT 0x01 /* The process is waiting on a child */
#define PROC_BLOCKED_COND 0x02 /* The thread is waiting on a condition */
#define PROC_BLOCKED_IO   0x03 /* The process is waiting on io to finish */
#define PROC_BLOCKED_SLEEP 0x04 /* The process is waiting on io to finish */

/* Reasons for io block */
#define PROC_IO_NONE     0x00 /* Not waiting on IO */
#define PROC_IO_KEYBOARD 0x01 /* Waiting for characters from the keyboard */

struct proc
{
	tty_t t; /* The tty this program is attached to. */
	pid_t sid; /* The session id*/
	pid_t pid; /* The id of the process */
	pid_t pgid; /* The process group */
	pid_t tid; /* The ID of this thread (if its a thread) */
	
	uid_t uid; /* The id of the user running the process */
	uid_t euid; /* The effective uid of the process */
	uid_t suid; /* The saved uid */
	uid_t ruid; /* The uid of the user that started the task. */
	
	gid_t gid; /* The id of the group running the process */
	gid_t egid; /* The effective gid of the process */
	gid_t sgid; /* The saved gid */
	gid_t rgid; /* The gid of the user that started the task. */

	mode_t umask; /* File creation mask */

	struct file_descriptor file_descriptors[MAX_FILES];
	
	uint state; /* The state of the process */
	
	/**
	 * Security note: The kernel should never try to access memory that
	 * is outside of:
	 *     ->   stack_end <= x < stack_start
	 *     ->   mmap_end <= x < mmap_start
	 *     ->   heap_end > x >= heap_start
	 *
	 * Accesses outside of these ranges could cause page faults or other
	 * types of faults.
	 */

	slock_t mem_lock;	
	/* Stack start will be greater than stack end */
	uint stack_start; /* The high memory, start of the stack */
	uint stack_end; /* Size of the stack in bytes. */
	/* Heap end will be greater than heap start. */
	uint heap_start; /* The low memory, start of the heap */
	uint heap_end; /* Size of the heap in bytes. */

	/* mmap area grows down towards heap */
	uint mmap_start; /* Start of the mmap area */
	uint mmap_end; /* end of the mmap area */

	uchar block_type; /* If blocked, what are you waiting for? */
	cond_t* b_condition; /* The condition we might be waiting on */
	uint b_condition_signal; /* The condition ticket number. */
	int b_pid; /* The pid we are waiting on. */
	int io_type; /* The type of io thats being waited on */
	uchar* io_dst; /* Where the destination bytes will be placed. */
	int io_request; /* Requested io size. Must be < PROC_IO_BUFFER */
	int io_recieved; /* The actual amount of bytes recieved. */
	uint sleep_time; /* The time when the sleep ends */
	struct signal_t* sig_queue; /* Signal queue*/
	void (*signal_handler)(int sig_num); /* Optional signal handler. */
	uint sig_stack_start; /* The start of the signal stack (higher) */
	uint sig_stack_end; /* The end of the signal stack (lower) */

	uchar orphan; /* Whether or not the parent has been killed. */
	int return_code; /* When the process finished, what did it return? */
	struct proc* parent; /* The process that spawned this process */
	char name[MAX_PROC_NAME]; /* The name of the process */
	char cwd[MAX_PATH_LEN]; /* Current working directory */

	pgdir* pgdir; /* The page directory for the process */
	uint* sys_esp; /* Pointer to the start of the syscall argv */
	uchar* k_stack; /* A pointer to the kernel stack for this process. */
	struct task_segment* tss; /* The task segment for this process */
	struct trap_frame* tf; /* A pointer to the trap frame from the int. */	
	uint entry_point; /* The address of the first instruction */
	uint context; /* The address at the top of the saved stack */

	uint user_ticks; /* The amount of ticks spent in user mode */
	uint kernel_ticks; /* The amount of ticks spent in kernel mode */
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
 * Check to see if the file looks like it is executable.
 */
uchar check_binary(const char* path);

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
 * Check to see if other processes are connected to this tty. Returns 0
 * if the tty is disconnected, 1 otherwise.
 */
uchar proc_tty_connected(tty_t t);

/**
 * Disconnect the given process from its tty.
 */
void proc_disconnect(struct proc* p);

/**
 * Disconnect all processes that are attached to the given tty.
 */
void proc_tty_disconnect(tty_t t);

/**
 * Set the controlling terminal for a process.
 */
void proc_set_ctty(struct proc* p, tty_t t);

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
