#ifndef _CHRONOS_H_
#define _CHRONOS_H_

/* Chronos traps */
#define TRAP_SC 0x80 /* System call trap*/

#define SYS_fork 	0x0
#define SYS_wait 	0x1
#define SYS_exec 	0x2
#define SYS_exit 	0x3
#define SYS_open 	0x4
#define SYS_close 	0x5
#define SYS_read 	0x6
#define SYS_write 	0x7

#define FSEEK_CUR	0x0
#define FSEEK_SET	0x1
#define FSEEK_END	0x2

#ifndef _SYSCALL_ASMONLY_
int fork(void);
int wait(int pid);
int exec(const char* path, const char** argv);
void exit(void) __attribute__ ((noreturn));
int open(const char* path);
int close(int fd);
int read(int fd, char* dst, uint sz); 
int write(int fd, char* dst, uint sz);
int fseek(int fd, int offset, int whence);
#endif

#endif
