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
 * Flag options for use in open:
 */
#define O_RDONLY  0x00000001  /* Open for reading */
#define O_WRONLY  0x00000002  /* Open for writing */
#define O_RDWR    (O_RDONLY | O_WRONLY)
#define O_CREATE  0x00000004  /* If the file has not been created, create it. */
#define O_CREAT   O_CREATE    /* Linux compatibility flag */
#define O_ASYNC   0x00000010  /* Enables signal-driven io. */
#define O_APPEND  0x00000020  /* Start writing to the end of the file. */
#define O_DIR     0x00000040  /* Fail to open the file unless it is a directory. */
#define O_DIRECTORY O_DIR     /* Linux compatibility */
#define O_NOATIME 0x00000100  /* Do not change access time when file is opened. */
#define O_TRUC    0x00000200  /* Once the file is opened, truncate the contents. */
#define O_SERASE  0x00000400  /* Securely erase files when deleting or truncating */
#define O_CLOEXEC 0x00000800  /* Close this file descriptor on a call to exec */
#define O_DIRECT  0x00001000  /* Minimize cache effects on the io from this file */
#define O_DSYNC   0x00002000  /* Write operations on the file complete immediatly*/
#define O_EXCL    0x00004000  /* Fail if this call doesn't create a file. */
#define O_LARGEFILE 0x00008000/* Allow files to be represented with off64_t */
#define O_NOCTTY  0x00010000  /* Do not allow the tty to become the proc's tty  */
#define O_NOFOLLOW 0x00020000 /* If the path is a symlink, fail to open.  */
#define O_NONBLOCK 0x0004000  /* Open the file in non blocking mode. */
#define O_NDELAY  O_NONBLOC   /* Linux Compatibility */
#define O_PATH    0x00100000  /* Only use the fd to represent a path */
#define O_SYNC    0x00200000  /* Don't return until metadata is pushed to disk  */
#define O_TMPFILE 0x00400000  /* Open the directory and create a tmp file */
#define O_TRUNC   0x00800000  /* If the file exists, truncate its contents. */


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
int open(const char* path, int flags, ...);

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
int fstat(int fd, struct stat* dst);

/**
 * Get a file stat structure from the given pathname. Returns 0 onsucess, -1
 * otherwise.
 */
int stat(const char* pathname, struct stat* dst);

/**
 * Just like stat except if pathname is a symbolic link, it returns
 * statistics on the link itself. Returns 0 on success, -1 otherwise.
 */
int lstat(const char* pathname, struct stat* dst);

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
int readdir(int fd, int index, struct dirent* dst);

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

/**
 * Set the program break (the address where the heap ends). Returns 0 on
 * success, -1 otherwise.
 */
int brk(void* addr);

/**
 * Increment the program break. Returns the original position of the
 * program break.
 */
void* sbrk(uint increment);

/**
 * Change the mode of a file. Returns 0 on success.
 */
int chmod(const char* path, mode_t perm);

/**
 * Change the ownership of a file. Returns 0 on success.
 */
int chown(const char* path, uint uid, uint gid);
#endif

#endif
