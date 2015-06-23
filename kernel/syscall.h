#ifndef _SYSCALL_H_
#define _SYSCALL_H_

int syscall_handler(uint* esp);
int sys_close(int fd);
int sys_fork(void);
int sys_wait(int pid);
int sys_exec(const char* path, const char** argv);
void sys_exit(void);
int sys_open(const char* path, int flags, int permissions);
int sys_close(int fd);
int sys_read(int fd, char* dst, uint sz);
int sys_write(int fd, char* src, uint sz);
int sys_lseek(int fd, int offset, int whence);
void* sys_mmap(void* hint, uint sz, int protection);
int sys_chdir(const char* dir);
int sys_cwd(char* dst, uint sz);
int sys_create(const char* file, uint permissions);
int sys_mkdir(const char* dir, uint permissions);
int sys_rmdir(const char* dir);
int sys_rm(const char* file);
int sys_mv(const char* orig, const char* dst);
int sys_fstat(int fd, struct file_stat* dst);
int sys_wait_s(struct cond* c, struct slock* lock);
int sys_wait_t(struct cond* c, struct tlock* lock);
int sys_signal(struct cond* c);
int sys_readdir(int fd, int index, struct directent* dst);

#endif
