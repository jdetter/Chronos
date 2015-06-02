#include "types.h"
#include "chronos.h"
#include "stdlock.h"
#include "vsfs.h"
#include "tty.h"
#include "proc.h"

extern slock_t ptable_lock; /* Process table lock */
extern struct proc* ptable; /* The process table */
extern struct proc* rproc; /* The currently running process */

int sys_fork(void)
{
	return 0;
}

int sys_wait(int pid)
{
	return 0;
}

int sys_exec(const char* path, const char** argv)
{
	return 0;
}

void sys_exit(void)
{
  int i;
  for(i = 0; i < MAXFILES; i++){
    if(rproc->file_descriptors[i]->type != 0x00){
      sys_close(i);
    }  
  }
  sched();       	
}

int sys_open(const char* path)
{
	return 0;
}

int sys_close(int fd)
{
  if(fd >= MAXFILES){return -1};
  rproc->file_descriptors[fd]->type = 0x00;
  rproc->file_descruptors[fd]->inode = NULL;  
  return 0;
}

int sys_read(int fd, char* dst, uint sz)
{
	return 0;
}

int sys_write(int fd, char* dst, uint sz)
{
	return 0;
}
