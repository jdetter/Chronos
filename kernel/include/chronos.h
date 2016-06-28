#ifndef _SYS_CHRONOS_H_
#define _SYS_CHRONOS_H_

/* Chronos traps */
#define TRAP_SC 		0x80 /* System call trap*/

#define SYS_fork 		0x01
#define SYS_wait 		0x02
#define SYS_exec 		0x03
#define SYS_exit 		0x04
#define SYS_open 		0x05
#define SYS_close 		0x06
#define SYS_read 		0x07
#define SYS_write 		0x08
#define SYS_lseek		0x09
#define SYS_mmap 		0x0A
#define SYS_chdir 		0x0B
#define SYS_getcwd 		0x0C
#define SYS_create 		0x0D
#define SYS_mkdir 		0x0E
#define SYS_rmdir 		0x0F
#define SYS_unlink		0x10
#define SYS_mv 			0x11
#define SYS_fstat 		0x12
#define SYS_wait_s		0x13
#define SYS_wait_t		0x14
#define SYS_signal_cv 	0x15
#define SYS_readdir		0x16
#define SYS_pipe		0x17
#define SYS_dup			0x18
#define SYS_dup2		0x19
#define SYS_proc_dump	0x1A
#define SYS_tty_mode	0x1B
#define SYS_tty_screen	0x1C
#define SYS_tty_cursor	0x1D
#define SYS_brk			0x1E
#define SYS_sbrk		0x1F
#define SYS_chmod		0x20
#define SYS_chown		0x21
#define SYS_mprotect	0x22
#define SYS__exit		0x23
#define SYS_execve		0x24
#define SYS_getpid		0x25
#define SYS_isatty		0x26
#define SYS_kill		0x27
#define SYS_link		0x28
#define SYS_stat		0x29
#define SYS_times		0x2A
#define SYS_gettimeofday 0x2B
#define SYS_waitpid		0x2C
#define SYS_creat		0x2D
#define SYS_getuid		0x2E
#define SYS_setuid		0x2F
#define SYS_getgid		0x30
#define SYS_setgid		0x31
#define SYS_gettid		0x32
#define SYS_getppid		0x33
#define SYS_munmap		0x34
#define SYS_getdents	0x35
#define SYS_getegid		0x36
#define SYS_geteuid		0x37
#define SYS_ioctl		0x38
#define SYS_access		0x39
#define SYS_ttyname		0x3A
#define SYS_fpathconf	0x3B
#define SYS_pathconf	0x3C
#define SYS_sleep		0x3D
#define SYS_umask		0x3E
#define SYS_lstat		0x3F
#define SYS_fchown		0x40
#define SYS_fchmod		0x41
#define SYS_gethostname	0x42
#define SYS_execl		0x43
#define SYS_utime		0x44
#define SYS_utimes		0x45
#define SYS_fcntl		0x46
#define SYS_sysconf		0x47
#define SYS_ftruncate	0x48
#define SYS_execvp		0x49
#define SYS_getpgid		0x4A
#define SYS_getresuid	0x4B
#define SYS_getresgid	0x4C
#define SYS_setresuid	0x4D
#define SYS_setpgid		0x4E
#define SYS_setresgid	0x4F
#define SYS_vfork		0x50
#define SYS_select		0x51
#define SYS_alarm		0x52
#define SYS_seteuid		0x53
#define SYS_sigaction	0x54
#define SYS_sigprocmask	0x55
#define SYS_sigpending	0x56
#define SYS_signal		0x57
#define SYS_sigsuspend	0x58
#define SYS_setegid		0x59
#define SYS_sync		0x5A
#define SYS_setreuid	0x5B
#define SYS_setregid	0x5C
#define SYS_reboot		0x5D

// Options for reboot system call
#define CHRONOS_RB_REBOOT 	0x01
#define CHRONOS_RB_SHUTDOWN 0x02

// #define SYS_semctl	0x5B
// #define SYS_semget	0x5C
// #define SYS_semop	0x5D
// #define SYS_semtimedop	0x5E

#ifndef __CHRONOS_ASM_ONLY__
#ifndef __ASM_ONLY__
extern int reboot(int type);

extern int __chronos_syscall(int num, ...);
#endif
#endif

#ifndef __CHRONOS_SYSCALLS_ONLY__
#ifndef __LINUX__

/**
 * Macros for pathconf
 */
#define _PC_LINK_MAX                      0
#define _PC_MAX_CANON                     1
#define _PC_MAX_INPUT                     2
#define _PC_NAME_MAX                      3
#define _PC_PATH_MAX                      4
#define _PC_PIPE_BUF                      5
#define _PC_CHOWN_RESTRICTED              6
#define _PC_NO_TRUNC                      7
#define _PC_VDISABLE                      8

/**
 * For use in function lseek: (Linux Compliant)
 *  + SEEK_SET Sets the current seek from the start of the file.
 *  + SEEK_CUR Adds the value to the current seek
 *  + SEEK_END Sets the current seek from the end of the file.
 */
/*#define SEEK_SET   0x00
#define SEEK_END   0x01
#define SEEK_CUR   0x02
#define SEEK_DATA  0x03
#define SEEK_HOLE  0x04 */

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
#define PROT_EXEC	0x04
#define PROT_GROWSDOWN  0x01000000
#define PROT_GROWSUP    0x02000000

/**
 * Signals
 */

// #define SIG_DFL ((_sig_func_ptr)0) /* Default action */
// #define SIG_IGN ((_sig_func_ptr)1) /* Ignore */
// #define SIG_ERR ((_sig_func_ptr)-1) /* Error return */

/* POSIX 1990 */
// #define SIGHUP		1  /* Terminate */
// #define SIGINT 		2  /* Terminate */
// #define SIGQUIT 	3  /* Terminate + Core dump */
// #define SIGILL		4  /* Terminate + Core dump */
// #define SIGABRT 	6  /* Terminate + Core dump */
// #define SIGFPE 		8  /* Terminate + Core dump */
// #define SIGKILL 	9  /* Terminate */
// #define SIGSEGV 	11 /* Terminate + Core dump */
// #define SIGPIPE 	13 /* Terminate */
// #define SIGALRM 	14 /* Terminate */
// #define SIGTERM 	15 /* Terminate */
// #define SIGUSR1 	10 /* Terminate */
// #define SIGUSR2 	12 /* Terminate */
// #define SIGCHLD 	17 /* Ignore */
// #define SIGCONT 	18 /* Continue */
// #define SIGSTOP 	19 /* Stop */
// #define SIGTSTP 	20 /* Stop */
// #define SIGTTIN 	21 /* Stop */
// #define SIGTTOU 	22 /* Stop */
// #define SIGSYS		31 /* Terminate + Core dump */

/* Various signals */
// #define SIGIOT		6  /* Terminate + Core dump */
// #define SIGEMT		0  /* Terminate - Not used on x86 */
// #define SIGSTKFLT	16 /* Terminate - Unused on x86 */
// #define SIGIO		29 /* Terminate */
// #define SIGCLD		0  /* Ignore - Not used on x86 */
// #define SIGPWR		30 /* Terminate */
// #define SIGINFO		SIGPWR /* Terminate */
// #define SIGLOST		0  /* Terminate - Unused on x86 */
// #define SIGWINCH	28 /* Ignore */
// #define SIGUNUSED	SIGSYS /* Terminate + Core dump */

/* POSIX signals */
// #define SIGBUS		7  /* Terminate + Core dump */
// #define SIGPOLL		SIGIO  /* Terminate */
// #define SIGPROF 	27 /* Terminate */
// #define SIGTRAP		5  /* Terminate + Core dump */
// #define SIGURG		23 /* Ignore */
// #define SIGVTALRM	26 /* Terminate */
// #define SIGXCPU		24 /* Terminate + Core dump */
// #define SIGXFSZ		25 /* Terminate + Core dump */

/**
 * Flags for access 
 */
#define R_OK  4               /* Test for read permission.  */
#define W_OK  2               /* Test for write permission.  */
#define X_OK  1               /* Test for execute permission.  */
#define F_OK  0               /* Test for existence.  */

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
// #define O_RDONLY  00000000  /* Open for reading */
// #define O_WRONLY  00000001  /* Open for writing */
// #define O_RDWR    00000002  /* Open for reading and writing */
// #define O_CREATE  00000100  /* If the file has not been created, create it. */
// #define O_CREAT   O_CREATE  /* Linux compatibility flag */
// #define O_EXCL    00000200  /* Fail if this call doesn't create a file. */
// #define O_NOCTTY  00000400  /* Do not allow the tty to become the proc's tty  */
// #define O_TRUNC   00001000 /* Once the file is opened, truncate the contents. */
// #define O_APPEND  00002000  /* Start writing to the end of the file. */
// #define O_NONBLOCK 0004000  /* Open the file in non blocking mode. */
// #define O_NDELAY  O_NONBLOC /* Linux Compatibility */
// #define O_SYNC    04010000  /* Don't return until metadata is pushed to disk  */
// #define O_FSYNC   O_SYNC    /* Linux compatibility. */
// #define O_ASYNC   00020000  /* Enables signal-driven io. */
// #define O_LARGEFILE 00100000/* Allow files to be represented with off64_t */
// #define O_DIR     00200000  /* Fail to open the file unless it is a directory. */
// #define O_DIRECTORY O_DIR   /* Linux compatibility */
// #define O_NOFOLLOW 00400000 /* If the path is a symlink, fail to open.  */
// #define O_CLOEXEC  02000000 /* Close this file descriptor on a call to exec */
// #define O_DIRECT   00040000 /* Minimize cache effects on the io from this file */
// #define O_NOATIME  01000000 /* Do not change access time when file is opened. */
// #define O_PATH    010000000 /* Only use the fd to represent a path */
//#define O_DSYNC   0010000   /* Write operations on the file complete immediatly*/
// #define O_TMPFILE 020200000 /* Open the directory and create a tmp file */

#define MAX_ARG		0x20

/**
 * Flags for fcntl
 */

/* fcntl(2) requests */
#define F_DUPFD         0       /* Duplicate a file descriptor */
#define F_GETFD         1       /* Get fildes flags (close on exec) */
#define F_SETFD         2       /* Set fildes flags (close on exec) */
#define F_GETFL         3       /* Get file flags */
#define F_SETFL         4       /* Set file flags */
#define F_GETOWN        5       /* Get owner - for ASYNC */
#define F_SETOWN        6       /* Set owner - for ASYNC */
#define F_GETLK         7       /* Get record-locking information */
#define F_SETLK         8       /* Set or Clear a record-lock (Non-Blocking) */
#define F_SETLKW        9       /* Set or Clear a record-lock (Blocking) */
#define F_RGETLK        10      /* Test a remote lock to see if it is blocked */
#define F_RSETLK        11      /* Set or unlock a remote lock */
#define F_CNVT          12      /* Convert a fhandle to an open fd */
#define F_RSETLKW       13      /* Set or Clear remote record-lock(Blocking) */
#define F_DUPFD_CLOEXEC 14      /* As F_DUPFD, but set close-on-exec flag */


#endif
#endif

#endif
