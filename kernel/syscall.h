#ifndef _SYSCALL_H_
#define _SYSCALL_H_

int syscall_handler(uint* esp);
int sys_close(int fd);
int sys_fork(void);
int sys_wait(int pid);
int sys_exec(const char* path, const char** argv);
void sys_exit(void);
int sys_open(const char* path);
int sys_close(int fd);
int sys_read(int fd, char* dst, uint sz);
int sys_write(int fd, char* src, uint sz);
int sys_lseek(int fd, int offset, int whence);
void* sys_mmap(void* hint, uint sz, int protection);

#endif
