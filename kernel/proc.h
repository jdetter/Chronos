#ifndef _PROC_H_
#define _PROC_H_

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

	struct proc* parent; /* The process that spawned this process */
	struct vsfs_inode* cwd; /* The current working directory */

	char name[MAX_PROC_NAME];
};

#endif
