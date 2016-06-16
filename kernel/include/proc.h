#ifndef _PROC_H_
#define _PROC_H_

/* Dependant headers */
#include <signal.h>
#include "context.h"
#include "fsman.h"
#include "ksignal.h"
#include "vm.h"
#include "trap.h"
#include "pipe.h"
#include "devman.h"
#include "tty.h"
#include "file.h"

/* The amount of processes in the ptable */
#define PTABLE_SIZE	0x40
#define FDS_TABLE_SZ	0x1000
#define MAX_PROC_NAME 	FILE_MAX_PATH
#define MAX_PATH_LEN	0x60

#define FD_TYPE_NULL 	0x00
#define FD_TYPE_INUSE 	0x01 /* In use but not initilized */
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
	int type; /* The type of this file descriptor */
	slock_t lock; /* The lock for this fd (read/write) */
	int refs; /* How many fdtabs reference this? */
	int flags; /* Any flags needed by the descriptor */
	int seek; /* What is the offset into the file? */
	inode i; /* The inode pointer */
	struct IODevice* device; /* The device driver (if dev) */
	pipe_t pipe; /* The pointer to the pipe (if pipe) */
	int pipe_type; /* The type of pipe (if pipe) */
	char path[FILE_MAX_PATH]; /* Mount point on the file system */
};

/* Maximum amount of file descriptors a process can have */
#define PROC_MAX_FDS 0x20

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
#define PROC_SIGNAL 	0x09 /* Process is waiting to receive a signal */

/* Blocked reasons */
#define PROC_BLOCKED_NONE 0x00 /* The process is not blocked */
#define PROC_BLOCKED_WAIT 0x01 /* The process is waiting on a child */
#define PROC_BLOCKED_COND 0x02 /* The thread is waiting on a condition */
#define PROC_BLOCKED_IO   0x03 /* The process is waiting on io to finish */
#define PROC_BLOCKED_SLEEP 0x04 /* The process is waiting on sleep  */

/* File descriptor table */
typedef struct file_descriptor** fdtab_t;

struct proc
{
	/** Process priviliges (all of the ids of this process) */
	pid_t sid; /* The session id*/
	pid_t pid; /* The id of the process */
	pid_t pgid; /* The process group */
	pid_t ppid; /* The id of the process that started this process */
	pid_t tid; /* The ID of this thread */
	pid_t tgid; /* The id of the main proces in this group */
	pid_t next_tid; /* The tid the next thread should take */
	uid_t uid; /* The id of the user running the process */
	uid_t euid; /* The effective uid of the process */
	uid_t suid; /* The saved uid */
	uid_t ruid; /* The uid of the user that started the task. */
	gid_t gid; /* The id of the group running the process */
	gid_t egid; /* The effective gid of the process */
	gid_t sgid; /* The saved gid */
	gid_t rgid; /* The gid of the user that started the task. */

	/** Open files for this process */
	fdtab_t fdtab;
	slock_t* fdtab_lock;
	mode_t umask; /* File creation mask */

	/** Process state parameters */
	int state; /* The state of the process */
	int block_type; /* If blocked, what are you waiting for? */
	cond_t* b_condition; /* The condition we might be waiting on */
	int b_condition_signal; /* The condition ticket number. */
	int b_pid; /* The pid we are waiting on. */
	time_t sleep_time; /* The time when the sleep ends */

	/** IO parameters */
	tty_t t; /* The tty this program is attached to. */
	char* io_dst; /* Where the destination bytes will be placed. */
	int io_request; /* Requested io size. Must be < PROC_IO_BUFFER */
	int io_recieved; /* The actual amount of bytes recieved. */
	struct proc* io_next; /* The next process waiting in the io queue*/
	void* io_ticket; /* Optional parameter for some io operations */

	/** SIGNAL stuff */
	int sig_handling; /* Are we handling a signal right now? */
	struct signal_t* sig_queue; /* Signal queue */
	struct trap_frame sig_saved; /* Saved registers while handling sig */
	struct sigaction sigactions[NSIG];
	uintptr_t sig_stack_start; /* The start of the signal stack (higher) */
	sigset_t sig_suspend_mask; /* The suspended mask */

	/** Parent and debug information */
	int orphan; /* Whether or not the parent has been killed. */
	int return_code; /* When the process finished, what did it return? */
	int wait_options; /* Parent wait options (waitpid) */
	int status_changed; /* Set by child, if set parent might wakeup */
	struct proc* parent; /* The process that spawned this process */
	char name[MAX_PROC_NAME]; /* The name of the process */
	char cwd[MAX_PATH_LEN]; /* Current working directory */

	/** Virtual memory */
	slock_t mem_lock;
	uintptr_t stack_start; /* The high memory, start of the stack */
	uintptr_t stack_end; /* Size of the stack in bytes. */
	uintptr_t heap_start; /* The low memory, start of the heap */
	uintptr_t heap_end; /* Size of the heap in bytes. */
	uintptr_t mmap_start; /* Start of the mmap area */
	uintptr_t mmap_end; /* end of the mmap area */
	uintptr_t code_start; /* Start of the code area */
	uintptr_t code_end; /* end of the code area */
	pgdir_t* pgdir; /* The page directory for the process */
	int* sys_esp; /* Pointer to the start of the syscall argv */
	context_t k_stack; /* A pointer to the kernel stack for this process. */
	struct task_segment* tss; /* The task segment for this process */
	struct trap_frame* tf; /* A pointer to the trap frame from the int.*/
	uintptr_t entry_point; /* The address of the first instruction */
	uintptr_t context; /* The address at the top of the saved stack */

	/* Resource usage */
	int user_ticks; /* The amount of ticks spent in user mode */
	int kernel_ticks; /* The amount of ticks spent in kernel mode */
};

extern struct proc ptable[];
extern struct proc* rproc;
extern slock_t ptable_lock;
extern pid_t next_pid;

extern context_t k_context;
extern pstack_t k_stack;

/**
 * Allocate a new process
 */
struct proc* alloc_proc();

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
 * Initilize a fdtab. This zeros all entries in the table.
 */
void fdtab_init(fdtab_t tab);

/**
 * Free the given file descriptor for the given process. This will clear the
 * fd table entry.
 */
void fd_free(struct proc* p, int fd);

/**
 * Frees all of the file descriptors in the given process's fdtab. Returns
 * 0 on success.
 */
int fd_tab_free(struct proc* p);

/**
 * Allocate a new file descriptor for a process. The result will be at pos
 * or after. If there is an error, -1 is returned. 0 is a valid file
 * descriptor.
 */
int fd_next_at(struct proc* p, int pos);

/**
 * Get any free file descriptor for the given process. On success, a
 * non negative integer is returned. Otherwise, -1 is returned.
 */
#define fd_next(process) fd_next_at(process, 0)

/**
 * Make dst and src share the same fdtab. Dst's fdtab will now point to
 * src's fd tab. Returns 0 on success, nonzero otherwise.
 */
int fd_tab_map(struct proc* dst, struct proc* src);

/**
 * Copy src's fdtab into dst's fdtab. All of the file descriptors in
 * dst's table are freed before they are overwritten. returns 0 on
 * success, nonzero otherwise.
 */
int fd_tab_copy(struct proc* dst, struct proc* src);

/**
 * Create a new file descriptor at the given index. free is whether or not 
 * the file descriptor should be freed if there was already a file descriptor
 * at the give position. Returns 0 if there was no file descriptor at this
 * index, otherwise returns 1 if there was a file descriptor at the given
 * position or -1 on error.
 */
int fd_new(struct proc* p, int index, int free);

/**
 * Function descriptor debugging function
 */
void fd_print_table(void);

/**
 * Wake up the parent of the given process if it's waiting on the given
 * process.
 */
extern void wake_parent(struct proc* p);

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

/**
 * Debug function
 */
void proc_print_table(void);

#endif
