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
#define SYS_getcwd 	0x0C
#define SYS_create 	0x0D
#define SYS_mkdir 	0x0E
#define SYS_rmdir 	0x0F
#define SYS_unlink	0x10
#define SYS_mv 		0x11
#define SYS_fstat 	0x12
#define SYS_wait_s	0x13
#define SYS_wait_t	0x14
#define SYS_signal_cv 	0x15
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
#define SYS_mprotect	0x22
#define SYS__exit	0x23
#define SYS_execve	0x24
#define SYS_getpid	0x25
#define SYS_isatty	0x26
#define SYS_kill	0x27
#define SYS_link	0x28
#define SYS_stat	0x29
#define SYS_times	0x2A
#define SYS_gettimeofday 0x2B
#define SYS_waitpid	0x2C
#define SYS_creat	0x2D
#define SYS_getuid	0x2E
#define SYS_setuid	0x2F
#define SYS_getgid	0x30
#define SYS_setgid	0x31
#define SYS_gettid	0x32
#define SYS_getppid	0x33
#define SYS_munmap	0x34
#define SYS_getdents	0x35
#define SYS_getegid	0x36
#define SYS_geteuid	0x37

/**
 * For use in function lseek: (Linux Compliant)
 *  + SEEK_SET Sets the current seek from the start of the file.
 *  + SEEK_CUR Adds the value to the current seek
 *  + SEEK_END Sets the current seek from the end of the file.
 */
#define SEEK_SET   0x00
#define SEEK_END   0x01
#define SEEK_CUR   0x02
#define SEEK_DATA  0x03
#define SEEK_HOLE  0x04

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
 * Protectections for mmap and mprotect
 */
#define PROT_NONE	0x00
#define PROT_READ	0x01
#define PROT_WRITE	0x02
#define PROT_EXE	0x04
#define PROT_GROWSDOWN  0x01000000
#define PROT_GROWSUP    0x02000000

/**
 * Signals
 */

/* POSIX 1990 */
#define SIGHUP		1  /* Terminate */
#define SIGINT 		2  /* Terminate */
#define SIGQUIT 	3  /* Terminate + Core dump */
#define SIGILL		4  /* Terminate + Core dump */
#define SIGABRT 	6  /* Terminate + Core dump */
#define SIGFPE 		8  /* Terminate + Core dump */
#define SIGKILL 	9  /* Terminate */
#define SIGSEGV 	11 /* Terminate + Core dump */
#define SIGPIPE 	13 /* Terminate */
#define SIGALRM 	14 /* Terminate */
#define SIGTERM 	15 /* Terminate */
#define SIGUSR1 	10 /* Terminate */
#define SIGUSR2 	12 /* Terminate */
#define SIGCHLD 	17 /* Ignore */
#define SIGCONT 	18 /* Continue */
#define SIGSTOP 	19 /* Stop */
#define SIGTSTP 	20 /* Stop */
#define SIGTTIN 	21 /* Stop */
#define SIGTTOU 	22 /* Stop */
#define SIGSYS		31 /* Terminate + Core dump */

/* Various signals */
#define SIGIOT		6  /* Terminate + Core dump */
#define SIGEMT		0  /* Terminate - Not used on x86 */
#define SIGSTKFLT	16 /* Terminate - Unused on x86 */
#define SIGIO		29 /* Terminate */
#define SIGCLD		0  /* Ignore - Not used on x86 */
#define SIGPWR		30 /* Terminate */
#define SIGINFO		SIGPWR /* Terminate */
#define SIGLOST		0  /* Terminate - Unused on x86 */
#define SIGWINCH	28 /* Ignore */
#define SIGUNUSED	SIGSYS /* Terminate + Core dump */

/* POSIX signals */
#define SIGBUS		7  /* Terminate + Core dump */
#define SIGPOLL		SIGIO  /* Terminate */
#define SIGPROF 	27 /* Terminate */
#define SIGTRAP		5  /* Terminate + Core dump */
#define SIGURG		23 /* Ignore */
#define SIGVTALRM	26 /* Terminate */
#define SIGXCPU		24 /* Terminate + Core dump */
#define SIGXFSZ		25 /* Terminate + Core dump */

/**
 * Flags for mmap.
 */
#define MAP_TYPE	0x0f 	/* Mask that determines the type of map */
#define MAP_SHARED 	0x01	/* Share this mapping (fork) */
#define MAP_PRIVATE	0x02	/* Set mapping to Copy on Write */
#define MAP_FIXED	0x10	/* Do not take addr as a hint. */
#define MAP_ANONYMOUS   0x20	/* Mapping is not backed by a file */
#define MAP_32BIT	0x40	/* Only map 32bit addresses */
#define MAP_ANON        MAP_ANONYMOUS /* DEPRICATED */
#define MAP_FILE 	0x00	/* DEPRICATED */
#define MAP_GROWSDOWN	0x00100	/* Used for stacks. */
#define MAP_DENYWRITE	0x00800	/* DEPRICATED */
#define MAP_EXECUTABLE	0x01000	/* DEPRICATED */
#define MAP_LOCKED	0x02000	/* Lock the page with mlock. */
#define MAP_NORESERVE	0x04000	/* Do not reserve swap space for this mapping. */
#define MAP_POPULATE	0x08000	/* Prepopulate the tlb. */
#define MAP_NONBLOCK	0x10000	/* Used with populate. */
#define MAP_STACK	0x20000	/* Marks allocation as a stack. */
#define MAP_HUGETLB	0x40000	/* Allocate huge pages. */
#define MAP_UNINITIALIZED 0x0	/* Disabled in Chronos. */


/**
 * Flag options for use in open. (Linux Compliant)
 */
#define O_RDONLY  00000000  /* Open for reading */
#define O_WRONLY  00000001  /* Open for writing */
#define O_RDWR    00000002  /* Open for reading and writing */
#define O_CREATE  00000100  /* If the file has not been created, create it. */
#define O_CREAT   O_CREATE  /* Linux compatibility flag */
#define O_EXCL    00000200  /* Fail if this call doesn't create a file. */
#define O_NOCTTY  00000400  /* Do not allow the tty to become the proc's tty  */
#define O_TRUNC    00001000 /* Once the file is opened, truncate the contents. */
#define O_APPEND  00002000  /* Start writing to the end of the file. */
#define O_NONBLOCK 0004000  /* Open the file in non blocking mode. */
#define O_NDELAY  O_NONBLOC /* Linux Compatibility */
#define O_SYNC    04010000  /* Don't return until metadata is pushed to disk  */
#define O_FSYNC   O_SYNC    /* Linux compatibility. */
#define O_ASYNC   00020000  /* Enables signal-driven io. */
#define O_LARGEFILE 00100000/* Allow files to be represented with off64_t */
#define O_DIR     00200000  /* Fail to open the file unless it is a directory. */
#define O_DIRECTORY O_DIR   /* Linux compatibility */
#define O_NOFOLLOW 00400000 /* If the path is a symlink, fail to open.  */
#define O_CLOEXEC  02000000 /* Close this file descriptor on a call to exec */
#define O_DIRECT   00040000 /* Minimize cache effects on the io from this file */
#define O_NOATIME  01000000 /* Do not change access time when file is opened. */
#define O_PATH    010000000 /* Only use the fd to represent a path */
#define O_DSYNC   0010000   /* Write operations on the file complete immediatly*/
#define O_TMPFILE 020200000 /* Open the directory and create a tmp file */

#define MAX_ARG		0x20

#ifndef __CHRONOS_ASM_ONLY__

/* Dependant headers */
#include "types.h"
#include "stdarg.h"
#include "file.h"
#include "stdlock.h"
#include "time.h"

/**
 * Fork the currently running process. The address space will be copied
 * as well as all open files and other resources. The child process will
 * return with 0 and the parent will return with the pid of the new child.
 */
int fork(void);

/**
 * Wait for a child process with the pid. If pid is -1, wait for any child 
 * process. If pid < -1, wait for any child whose process group id is equal
 * to the absolute value of pid. 0 means wait for any child process whose
 * process group id is equal to that of the calling process. > 0 means wait
 * for a child process whose pid is equal to the specified pid. The status 
 * argument should be specified for the resulting exit code of the child.
 * Returns the pid of the process that was waited on.
 */
int waitpid(int pid, int* status, int options);

/**
 * Replace the currently running process with the process designated by the
 * path. Use the arguments argv when starting the process.
 */
int exec(const char* path, const char** argv);

/**
 * End the currently running task. This means that all files open by the task
 * will be freed up as well as all memory in use by the program. This function
 * does not return.
 */
void exit(int retcode) __attribute__ ((noreturn));

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
int getcwd(char* dst, size_t sz);

/**
 * Create the file with the given permissions. If this call is successfull,
 * the file will be created with the permissions specified. If unsuccessful,
 * the file will not be created and -1 will be returned. Otherwise, 0 is
 * returned.
 */
int create(const char* file, mode_t mode);

/**
 * See create. Linux compatibility function.
 */
int creat(const char* file, mode_t mode);

/**
 * Create the directory specified by the path. The directory will have the
 * specified permissions. If the path is invalid or more than one directory
 * needs to be created to make the path valid, -1 will be returned and the
 * directory will not be created. Otherwise the file will be created and 
 * opened and a new file descriptor for the file will be returned.
 */
int mkdir(const char* dir, mode_t mode);

/**
 * Remove the specified directory. The directory must be empty besides the
 * entries . and .. , on success 0 is returned otherwise -1 is returned.
 */
int rmdir(const char* dir);

/**
 * Remove the specified file. If the file is removed, 0 is returned. Otherwise
 * -1 is returned.
 */
int unlink(const char* file);

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
int signal_cv(struct cond* c);

/**
 * Reads a directory entry from the file fd into dirp. 1 is returned
 * on success, 0 is returned on end of directory. -1 is returned on
 * failure. Count is ignored (compatibility).
 */
int readdir(int fd, struct old_linux_dirent* dirp, uint count);

/**
 * Better method of obtaining directory entries. Count specifies how
 * many entries can be read at a time into the dirp buffer from the open
 * file descriptor fd. Returns the number of bytes read on success, 0
 * on end of diretory and -1 on error.
 */
int getdents(int fd, struct dirent* dirp, uint count);

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

/**
 * Change the protection on a segment in memory. Returns 0 on success.
 * See protections above.
 */
int mprotect(void* addr, size_t len, int prot);

/**
 * Exit from the current task. This version of exit doesn't close open files.
 * Any open file descriptors will be leaked. This function doesn't return.
 */
void _exit(int retcode)  __attribute__ ((noreturn));

/**
 * Execute the program specified by filename. The program will be executed
 * with the arguments specified in argv. The program will be executed with
 * the environment variables specified in envp. argv and envp are null terminated.
 */
int execve(const char* filename, char* const argv[], char* const envp[]);

/**
 * Returns the pid of the running process.
 */
int getpid(void);

/**
 * Returns 1 if the fd is actually a tty. Returns 0 if the fd is
 * not a tty. Returns -1 on error.
 */
int isatty(int fd);

/**
 * Send a signal to a process with the specified pid. Returns 0 on sucess, 
 * -1 otherwise.
 */
int kill(pid_t pid, int sig);

/**
 * Create a new hardlink to the file specified by oldpath. The hard link
 * will be created at newpath. Returns 0 on sucess. Returns -1 otherwise.
 */
int link(const char* oldpath, const char* newpath);

/**
 * Stats the file specified by path. The result of the stat will be parsed
 * into the stat structure dst. Returns 0 on success, -1 otherwise.
 */
int stat(const char* path, struct stat* dst);

/**
 * Gets process times. The results are measured in kernel ticks. Returns
 * The number of ticks that have occurred since the system has booted.
 */
clock_t times(struct tms* dst);

/**
 * Get the current time of day and parse it into the struct tv. The use of
 * a timezone is ignored by Chronos. The tv struct will be parsed with the
 * current time parameters. Returns 0 on success, -1 otherwise.
 */
int gettimeofday(struct timeval* tv, struct timezone* tz);

/**
 * Returns the amount of seconds since Epoch (January 1st, 1970 UTC 00:00:00).
 * If t is non-NULL the result will also be stored in t. On success, the
 * time in seconds since the Epoch is returned, otherwise returns -1.
 */
time_t time(time_t* t);

/**
 * Wait for a child process to exit. On success, returns the pid of the
 * process that exited and puts the exit code into status. On failure,
 * -1 is returned.
 */
int wait(int* status);

/**
 * Returns the uid of the calling process.
 */
pid_t getuid(void);

/**
 * Sets the uid of the calling process. Return 0 on success, -1 
 * otherwise.
 */
int setuid(uid_t uid);

/**
 * Returns the gid of the calling process.
 */
gid_t getgid(void);

/**
 * Sets the gid of the calling process. Returns 0 on success, -1
 * otherwise.
 */
int setgid(gid_t gid);

/**
 * Returns the tid of the calling process (thread).
 */
pid_t gettid(void);

/**
 * Returns the pid of the parent of the calling process.
 */
pid_t getppid(void);

/**
 * Unmap a region in memory. This does not close the mapped file if there
 * is a mapped file. Returns 0 on success, -1 otherwise.
 */
int munmap(void* addr, size_t length);

/**
 * Returns the effective group id of the process.
 */
gid_t getegid(void);

/**
 * Returns the effective user id of the process.
 */
uid_t geteuid(void);

#endif

#endif
