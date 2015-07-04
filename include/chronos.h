#ifndef _CHRONOS_H_
#define _CHRONOS_H_

/* Chronos traps */
#define TRAP_SC 	0x80 /* System call trap*/

#define SYS_fork 	0x01
#define SYS_wait 	0x02
#define SYS_exec 	0x03
#define SYS_exit 	0x04
#define SYS_open 	0x05
#define SYS_close 	0x06
#define SYS_read 	0x07
#define SYS_write 	0x08
#define SYS_lseek	0x09
#define SYS_mmap 	0x0A
#define SYS_chdir 	0x0B
#define SYS_cwd 	0x0C
#define SYS_create 	0x0D
#define SYS_mkdir 	0x0E
#define SYS_rmdir 	0x0F
#define SYS_rm 		0x10
#define SYS_mv 		0x11
#define SYS_fstat 	0x12
#define SYS_wait_s	0x13
#define SYS_wait_t	0x14
#define SYS_signal 	0x15
#define SYS_readdir	0x16
#define SYS_pipe	0x17
#define SYS_dup		0x18
#define SYS_dup2	0x19
#define SYS_proc_dump	0x1A
#define SYS_tty_mode	0x1B
#define SYS_tty_screen	0x1C
#define SYS_tty_cursor	0x1D
#define SYS_brk		0x1E
#define SYS_sbrk	0x1F
#define SYS_chmod	0x20
#define SYS_chown	0x21

/**
 * For use in function lseek:
 *  + SEEK_SET Sets the current seek from the start of the file.
 *  + SEEK_CUR Adds the value to the current seek
 *  + SEEK_END Sets the current seek from the end of the file.
 */
#define SEEK_SET 0x01
#define SEEK_END 0x02
#define SEEK_CUR 0x04
#define SEEK_DATA 0x08
#define SEEK_HOLE 0x10

/* Segment descriptions */
/* Null segment		0x00 */
#define SEG_KERNEL_CODE	0x01
#define SEG_KERNEL_DATA 0x02
#define SEG_USER_CODE   0x03 /* data must be 1 away from code (sysexit) */
#define SEG_USER_DATA   0x04
#define SEG_TSS		0x05

#define SEG_COUNT 	0x06

#define SYS_EXIT_BASE	((SEG_USER_CODE << 3) - 16)

/**
 * Protectections for mmap
 */
#define PROT_NONE	0x00
#define PROT_EXE	0x01
#define PROT_READ	0x02
#define PROT_WRITE	0x04

/**
 * Flags for mmap.
 */
#define MAP_ANONYMOUS   0x01	/* Mapping is not backed by a file */
#define MAP_ANON        MAP_ANONYMOUS /* DEPRICATED */
#define MAP_SHARED 	0x02	/* Share this mapping (fork) */
#define MAP_PRIVATE	0x03	/* Set mapping to Copy on Write */
#define MAP_DENYWRITE	0x04	/* Ignored */
#define MAP_EXECUTABLE 	0x05	/* Ignored */
#define MAP_FILE 	0x06	/* Ignored */
#define MAP_FIXED	0x07	/* Do not take addr as a hint. */
#define MAP_GROWSDOWN	0x08	/* Used for stacks. */
#define MAP_MAP_HUGETLB 0x09	/* Huge pages (ignored ) */
#define MAP_HUGE_2MB	0x0A	/* 2MB pages (ignored) */
#define MAP_HUGE_1GB	0x0B	/* 1GB pages (ignored) */
#define MAP_LOCKED	0x0C	/* Lock the pages of this mapped region */
#define MAP_NONBLOCK	0x0D	/* Ignored for now */
#define MAP_NORESERVE	0x0E	/* Do not reserve swap space */
#define MAP_32BIT	0x0F	/* Compatibility flag */
#define MAP_POPULATE	0x10	/* Prefault page tables */
#define MAP_STACK	0x11	/* Suitable for thread stacks */
#define MAP_UNINITIALIZED 0	/* Ignored by Chronos */

/**
 * Permissions for files. This is to be used with create and chmod. 
 */
#define PERM_UREAD      00100
#define PERM_UWRITE     00200
#define PERM_UEXEC      00400

#define PERM_GREAD      00010
#define PERM_GWRITE     00020
#define PERM_GEXEC      00040

#define PERM_OREAD	00001
#define PERM_OWRITE	00002
#define PERM_OEXEC	00004

#define MAX_ARG		0x20

#ifndef __CHRONOS_ASM_ONLY__

/* Dependant headers */
#include "types.h"
#include "stdarg.h"
#include "file.h"
#include "stdlock.h"

/**
 * Fork the currently running process. The address space will be copied
 * as well as all open files and other resources. The child process will
 * return with 0 and the parent will return with the pid of the new child.
 */
int fork(void);

/**
 * Wait for a child process with the pid. If pid is -1, wait for any child
 * to exit. Only parents can wait for their children. Children of other
 * processes cannot be waited on.
 */
int wait(int pid);

/**
 * Replace the currently running process with the process designated by the
 * path. Use the arguments argv when starting the process.
 */
int exec(const char* path, const char** argv);

/**
 * End the currently running task. This means that all files open by the task
 * will be freed up as well as all memory in use by the program.
 */
void exit(void) __attribute__ ((noreturn));

/**
 * Open the file designated by the path. The resulting file descriptor will
 * be returned if the file exists and the user is allowed to open the file.
 */
int open(const char* path, int flags, int permissions);

/**
 * Close the file descriptor fd. This allows the file descriptor to be reused.
 * it also allows other programs to use the resources freed up after this call.
 */
int close(int fd);

/**
 * Read from the file descriptor. The destination buffer will be completely
 * filled before returning, unless there is an error. The function returns
 * the amount of bytes read into dst. If the end of the file is reached, -1
 * is returned.
 */
int read(int fd, void* dst, size_t sz);

/**
 * Write to a file descriptor. sz bytes will be written to the file descriptor.
 * This function returns the amount of bytes written to the file descriptor.
 */ 
int write(int fd, void* src, size_t sz);

/**
 * Seek the file descriptor to the point offset. See the lseek definitions
 * above. Returns the resulting position of the file pointer.
 */
int lseek(int fd, off_t offset, int whence);

/**
 * Map memory into user memory.
 */
void* mmap(void* addr, size_t sz, int prot, 
		int flags, int fd, off_t offset);

/**
 * Change the current working directory to dir. The path can be relative or
 * absolute. Returns -1 if the directory is invalid or doesn't exist. Returns
 * 0 on success.
 */
int chdir(const char* dir);

/** 
 * Get the current working directory of this process. The working directory
 * will be read into the destination buffer for a maximum of sz characters.
 * The path is guarenteed to be null terminated. This function returns the
 * amount of characters put into the destination buffer. Returns -1 if the
 * buffer is not large enough to hold the path, 0 otherwise.
 */
int cwd(char* dst, uint sz);

/**
 * Create the file with the given permissions. If this call is successfull,
 * the file will be created with the permissions specified. If unsuccessful,
 * the file will not be created and -1 will be returned. Otherwise, 0 is
 * returned.
 */
int create(const char* file, uint permissions);

/**
 * Create the directory specified by the path. The directory will have the
 * specified permissions. If the path is invalid or more than one directory
 * needs to be created to make the path valid, -1 will be returned and the
 * directory will not be created. Otherwise the file will be created and 
 * opened and a new file descriptor for the file will be returned.
 */
int mkdir(const char* dir, uint permissions);

/**
 * Remove the specified directory. The directory must be empty besides the
 * entries . and .. , on success 0 is returned otherwise -1 is returned.
 */
int rmdir(const char* dir);

/**
 * Remove the specified file. If the file is removed, 0 is returned. Otherwise
 * -1 is returned.
 */
int rm(const char* file);

/**
 * Move the specified file from orig to dst. If the file is moved, 0 is
 * returned, otherwise -1 is returned.
 */
int mv(const char* orig, const char* dst);

/**
 * Read that stats from file path and read them into dst. Returns 0 on sucess,
 * -1 on failure.
 */
int fstat(int fd, struct file_stat* dst);

/**
 * Get a file stat structure from the given pathname. Returns 0 onsucess, -1
 * otherwise.
 */
int stat(const char* pathname, struct file_stat* dst);

/**
 * Just like stat except if pathname is a symbolic link, it returns
 * statistics on the link itself. Returns 0 on success, -1 otherwise.
 */
int lstat(const char* pathname, struct file_stat* dst);

/**
 * Wait on the given condition variable c and release the spin lock.
 */
int wait_s(struct cond* c, struct slock* lock);

/**
 * Wait on the given condition variable c and release the ticket lock.
 */
int wait_t(struct cond* c, struct tlock* lock);

/**
 * Signal (single wakeup) a condition variable.
 */
int signal(struct cond* c);

/**
 * Read from the file descriptor directory fd. The directory entry at index
 * index will be read into dst. If you have reached the end of the
 * directory, -1 is returned. Otherwise 0 is returned.
 */
int readdir(int fd, int index, struct directent* dst);

/**
 * Creates a new pipe. A pipe is a buffer that can be read from and written
 * to. Pipes exist across fork calls which allows processes to communicate
 * with eachother. Pipe puts the file descriptor for the read end of the
 * pipe into the first position of the array and then puts the file descriptor
 * for the write end of the pipe into the second position of the array.
 */
int pipe(int fd[2]);

/**
 * Duplicates the given file descriptor. Returns the new file descriptor. If
 * the file descriptor could not be duplicated, returns -1.
 */
int dup(int fd);

/**
 * Assigns an old file descriptor to a new file descriptor number. If the new 
 * file descriptor number is valid and not closed, it will be close. The old
 * file descriptor is not closed. If the old file descriptor is not valid, no
 * operation is performed and -1 is returned. Otherwise, 0 is returned.
 */
int dup2(int fd_old, int fd_new);

/**
 * Prints out process information to the screen.
 */
int proc_dump(void);

/**
 * Sets whether or not the tty that is the running process is attached to is
 * in graphical mode. graphical = 1 puts the tty into graphical mode,
 * graphical = 0 puts the tty back into text mode. Always returns 0.
 */
int tty_mode(int graphical);

/**
 * Prints the given buffer into the graphical buffer of the tty. The buffer
 * is expected to be 4000 bytes long. Always returns 0.
 */
int tty_screen(char tty_buffer[4000]);

/**
 * Sets the graphical cursor position.
 */
int tty_cursor(int pos);
#endif

#endif
