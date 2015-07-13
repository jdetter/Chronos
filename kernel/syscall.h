#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/* syscall utility functions */
/**
 * Checks whether or not the pointer points to a valid byte in memory.
 * Returns 0 if the address is ok, 1 otherwise.
 */
uchar syscall_addr_safe(void* address);

/**
 * Checks to see if the pointer is safe to access. Returns 0 if the
 * pointer is safe to access, 1 otherwise.
 */
uchar syscall_ptr_safe(void* address);

/**
 * Gets an int argument from the stack of the running process.
 */
int syscall_get_int(int* dst, uint arg_num);

/**
 * Take the buffer from the user stack and put it into dst. sz_user
 * specifies the amount of byes to copy from the user stack. sz_kern
 * specifies the size of the destination buffer. arg specifies which
 * position the argument is at on the stack. Returns 0 on success,
 * 1 otherwise.
 */
uchar syscall_get_buffer(void* dst, uint sz_user, uint sz_kern, uint arg);

/**
 * Get a string from a user stack and put it into dst. A maximum amount
 * of sz_kern bytes will be copied. arg_num specifies the argument
 * position on the stack.
 */
uchar syscall_get_str(char* dst, uint sz_kern, uint arg_num);


/**
 * Puts a pointer to a buffer into ptr. The buffer will be verified to
 * be at least sz bytes long. Returns 0 on sucess, 1 otherwise.
 */
uchar syscall_get_buffer_ptr(void** ptr, uint sz, uint arg_num);

/**
 * Get a pointer to a list of pointers. The pointer addresses are NOT
 * checked for validity. Returns 0 on success, 1 otherwise.
 */
uchar syscall_get_buffer_ptrs(uchar*** ptr, uint arg_num);

/**
 * Safetly get a pointer to a string on the user stack. The pointer
 * to the start of the string will be placed into dst. arg_num is
 * the position on the stack of the pointer. Returns 0 on sucess,
 * 1 otherwise.
 */
uchar syscall_get_str_ptr(const char** dst, uint arg_num);

int syscall_handler(uint* esp);
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
int sys_signal(void);
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


#define SYS_MIN SYS_fork /* System call with the smallest value */
#define SYS_MAX SYS_creat /* System call with the greatest value*/

#endif
