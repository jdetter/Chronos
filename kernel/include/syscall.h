#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/* syscall utility functions */

/**
 * Checks whether or not the pointer points to a valid byte in memory.
 * Returns 0 if the address is ok, 1 otherwise.
 */
int syscall_addr_safe(void* address);

/**
 * Checks to see if the pointer is safe to access. Returns 0 if the
 * pointer is safe to access, 1 otherwise.
 */
int syscall_ptr_safe(void* address);

/**
 * Gets an int argument from the stack of the running process.
 */
int syscall_get_int(int* dst, int arg_num);

/**
 * Gets a long argument from the stack of the running process.
 */
int syscall_get_long(long* dst, int arg_num);

/**
 * Gets a long argument from the stack of the running process.
 */
int syscall_get_short(short* dst, int arg_num);

/**
 * Get a string from a user stack and put it into dst. A maximum amount
 * of sz_kern bytes will be copied. arg_num specifies the argument
 * position on the stack.
 */
int syscall_get_str(char* dst, int sz_kern, int arg_num);


/**
 * Puts a pointer to a buffer into ptr. The buffer will be verified to
 * be at least sz bytes long. Returns 0 on sucess, 1 otherwise.
 */
int syscall_get_buffer_ptr(void** ptr, int sz, int arg_num);

/**
 * Get a pointer to a list of pointers. The pointer addresses are NOT
 * checked for validity. Returns 0 on success, 1 otherwise.
 */
int syscall_get_buffer_ptrs(void*** ptr, int arg_num);

/**
 * Safetly get a pointer to a string on the user stack. The pointer
 * to the start of the string will be placed into dst. arg_num is
 * the position on the stack of the pointer. Returns 0 on sucess,
 * 1 otherwise.
 */
int syscall_get_str_ptr(const char** dst, int arg_num);

/**
 * Probe an argument on the stack. If the pointer is reachable
 * and non zero, 0 is returned. Otherwise 1 is returned and
 * ptr is undefined.
 */
int syscall_get_optional_ptr(void** ptr, int arg_num);

/* Check to see if an fd is valid */
int fd_ok(int fd);

/**
 * Find an available file descriptor that is > val
 */
int find_fd_gt(int val);

int syscall_handler(int* esp);
int sys_close(void);
int sys_fork(void);
int sys_wait(void);
int sys_exec(void);
int sys_exit(void);
int sys_open(void);
int sys_close(void);
int sys_read(void);
int sys_write(void);
int sys_lseek(void);
int sys_mmap(void);
int sys_chdir(void);
int sys_getcwd(void);
int sys_create(void);
int sys_creat(void);
int sys_mkdir(void);
int sys_rmdir(void);
int sys_unlink(void);
int sys_mv(void);
int sys_fstat(void);
int sys_wait_s(void);
int sys_wait_t(void);
int sys_signal_cv(void);
int sys_readdir(void);
int sys_pipe(void);
int sys_dup(void);
int sys_dup2(void);
int sys_proc_dump(void);
int sys_tty_mode(void);
int sys_tty_screen(void);
int sys_tty_cursor(void);
int sys_brk(void);
int sys_sbrk(void);
int sys_chmod(void);
int sys_chown(void);
int sys_mprotect(void);
int sys__exit(void);
int sys_execve(void);
int sys_getpid(void);
int sys_isatty(void); /* newlib specific */
int sys_kill(void);
int sys_link(void);
int sys_stat(void);
int sys_times(void);
int sys_gettimeofday(void);
int sys_waitpid(void);
int sys_getuid(void);
int sys_setuid(void);
int sys_getgid(void);
int sys_setgid(void);
int sys_gettid(void);
int sys_getppid(void);
int sys_times(void);
int sys_munmap(void);
int sys_getdents(void);
int sys_getegid(void);
int sys_geteuid(void);
int sys_ioctl(void);
int sys_access(void);
int sys_ttyname(void);
int sys_fpathconf(void);
int sys_pathconf(void);
int sys_sleep(void);
int sys_umask(void);
int sys_lstat(void);
int sys_fchown(void);
int sys_fchmod(void);
int sys_gethostname(void);
int sys_execl(void);
int sys_utime(void);
int sys_utimes(void);
int sys_fcntl(void);
int sys_sysconf(void);
int sys_ftruncate(void);
int sys_execvp(void);
int sys_getpgid(void);
int sys_getresuid(void);
int sys_getresgid(void);
int sys_setresuid(void);
int sys_setpgid(void);
int sys_setresgid(void);
int sys_vfork(void);
int sys_select(void);
int sys_alarm(void);
int sys_sigaction(void);
int sys_sigprocmask(void);
int sys_sigpending(void);
int sys_signal(void);
int sys_sigsuspend(void);
int sys_setegid(void);
int sys_sync(void);
int sys_setreuid(void);
int sys_setregid(void);
int sys_reboot(void);

#include <chronos.h>

#define SYS_MIN SYS_fork /* System call with the smallest value */
#define SYS_MAX SYS_reboot /* System call with the greatest value*/

#endif
