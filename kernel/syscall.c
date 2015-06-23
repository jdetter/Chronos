#include "types.h"
#include "x86.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "proc.h"
#include "vm.h"
#include "chronos.h"
#include "stdlock.h"
#include "vsfs.h"
#include "tty.h"
#include "elf.h"
#include "stdarg.h"
#include "stdlib.h"
#include "syscall.h"
#include "serial.h"
#include "trap.h"
#include "panic.h"

extern uint next_pid;
extern slock_t ptable_lock; /* Process table lock */
extern struct proc ptable[]; /* The process table */
extern struct proc* rproc; /* The currently running process */

/* Is the given address safe to access? */
uchar syscall_addr_safe(uchar* address)
{
	if((rproc->stack_end >= (uint)address 
		&& (uint)address < rproc->stack_start) ||
		   (rproc->heap_end < (uint)address 
		&& (uint)address >= rproc->heap_start))
	{
		/* Valid memory access */
		return 0;
	}
	/* Invalid memory access */
	return 1;
}

uchar syscall_ptr_safe(uchar* address)
{
	if(syscall_addr_safe(address) 
		|| syscall_addr_safe(address + 3))
			return 1;
	return 0;
}

/**
 * Get an integer argument. The arg_num determines the offset to the argument.
 */
int syscall_get_int(int* dst, uint* esp, uint arg_num)
{
	esp += arg_num;
	uchar* num_start = (uchar*)esp;
	uchar* num_end = (uchar*)esp + 3;

	if(syscall_addr_safe(num_start) || syscall_addr_safe(num_end))
		return 1;

	*dst = *esp;
	return 0;
}

/**
 * Get a string argument. The arg_num determines the offset to the argument.
 */
int syscall_get_buffer(char* dst, uint sz_user, uint sz_kern, 
		uint* esp, uint arg_num)
{
	memset(dst, 0, sz_kern);
	esp += arg_num; /* This is a pointer to the string we need */
	uchar* str_addr = (uchar*)esp;
	if(syscall_ptr_safe(str_addr))
		return 1;
	uchar* str = *(uchar**)str_addr;
	if(syscall_addr_safe(str) || syscall_addr_safe(str + sz_user))
		return 1;
	
	uint sz = sz_user;
	if(sz_kern < sz_user)
		sz = sz_kern;
	int x;
	for(x = 0;x < sz;x++)
	{
		if(str[x] == 0)
		{
			dst[x] = 0;
			break;
		}

		dst[x] = str[x];
	}

	return 0;
}

int syscall_get_str(char* dst, uint sz_kern,
                uint* esp, uint arg_num)
{
        memset(dst, 0, sz_kern);
        esp += arg_num; /* This is a pointer to the string we need */
        uchar* str_addr = (uchar*)esp;
        if(syscall_ptr_safe(str_addr))
                return 1;
        uchar* str = *(uchar**)str_addr;

        int x;
        for(x = 0;x < sz_kern;x++)
        {
		if(syscall_ptr_safe(str + x))
			return 1;
                dst[x] = str[x];
                if(str[x] == 0) break;
        }

        return 0;
}


int syscall_get_buffer_ptr(char** ptr, uint sz, uint* esp, uint arg_num)
{
	esp += arg_num; /* This is a pointer to the string we need */
	uchar* buff_addr = (uchar*)esp;
	if(syscall_addr_safe(buff_addr))
                return 1;
	uchar* buff = *(uchar**)buff_addr;
        if(syscall_addr_safe(buff) || syscall_addr_safe(buff + sz))
                return 1;

	*ptr = (char*)buff;

	return 0;
}

int syscall_get_buffer_ptrs(uchar*** ptr, uint* esp, uint arg_num)
{
	esp += arg_num;
	/* Get the address of the buffer */
	uchar*** buff_buff_addr = (uchar***)esp;
	/* Is the user stack okay? */
	if(syscall_ptr_safe((uchar*) buff_buff_addr)) 
		return 1;
	uchar** buff_addr = *buff_buff_addr;
	/* Check until we hit a null. */
	int x;
	for(x = 0;x < MAX_ARG;x++)
	{
		if(syscall_ptr_safe((uchar*)(buff_addr + x)))
			return 1;
		uchar* val = *(buff_addr + x);
		if(val == 0) break;
	}

	if(x == MAX_ARG)
	{
		/* the list MUST end with a NULL element! */
		return 1;
	}

	/* Everything is reasonable. */
	*ptr = buff_addr;
	return 0;
}


int syscall_handler(uint* esp)
{
	/* Get the number of the system call */
	int syscall_number = -1;
	int return_value = -1;
	if(syscall_get_int(&syscall_number, esp, 0)) return 1;
	esp++;
	int int_arg1;
	int int_arg2;
	int int_arg3;

#define SYSCALL_STRLEN 512
	char str_arg1[SYSCALL_STRLEN];
	//char str_arg2[SYSCALL_STRLEN];

	char** arg_list;

	char* buff_ptr1;

	switch(syscall_number)
	{
		case SYS_fork:
			return_value = sys_fork();
			break;
		case SYS_wait:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			return_value = sys_wait(int_arg1);
			break;
		case SYS_exec:
			return_value = 0;
			if(syscall_get_buffer_ptr(&buff_ptr1, 
				int_arg2, esp, 1)) break;
			if(syscall_get_buffer_ptrs(
				(uchar***)&arg_list, esp, 2))
				break;
			sys_exec(buff_ptr1, (const char**)arg_list);
			break;
		case SYS_exit:
			sys_exit();
			break;
		case SYS_open:
			if(syscall_get_str((char*)str_arg1,  
                                SYSCALL_STRLEN, esp, 1)) break;	
			if(syscall_get_int(&int_arg1, esp, 2)) break;
                        if(syscall_get_int(&int_arg2, esp, 3)) break;
			return_value = sys_open(str_arg1, 
				int_arg1, int_arg2);
			break;
		case SYS_close:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			return_value = sys_close(int_arg1);
			break;
		case SYS_read:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			if(syscall_get_int(&int_arg2, esp, 3)) break;
			if(syscall_get_buffer_ptr(&buff_ptr1, 
				int_arg2, esp, 2)) break;
			return_value = sys_read(int_arg1, buff_ptr1, int_arg2);	
			break;
		case SYS_write:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			if(syscall_get_int(&int_arg2, esp, 3)) break;
			if(syscall_get_buffer((char*)str_arg1, int_arg2, 
				SYSCALL_STRLEN, esp, 2)) break;
			return_value = sys_write(int_arg1, str_arg1, int_arg2);
			break;
		case SYS_lseek:
                        if(syscall_get_int(&int_arg1, esp, 1)) break;
                        if(syscall_get_int(&int_arg2, esp, 2)) break;
                        if(syscall_get_int(&int_arg3, esp, 3)) break;
			return_value = sys_lseek(int_arg1, int_arg2, int_arg3);
			break;
		case SYS_mmap:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			if(syscall_get_int(&int_arg2, esp, 2)) break;
			if(syscall_get_int(&int_arg3, esp, 3)) break;
			return_value = (int)sys_mmap((void*)int_arg1,
				(uint)int_arg2, int_arg3);
			break;
		case SYS_chdir:
			break;
		case SYS_cwd:
			break;
		case SYS_create:
			break;
		case SYS_mkdir:
			break;
		case SYS_rmdir:
			break;
		case SYS_rm:
			break;
		case SYS_mv:
			break;
		case SYS_fstat:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			if(syscall_get_buffer(str_arg1, sizeof(struct file_stat), 
				SYSCALL_STRLEN, esp, 2)) break;
			if(syscall_get_int(&int_arg2, esp, 2)) break;
			return_value = sys_fstat(int_arg1, 
				(struct file_stat*)int_arg2);
			break;
		case SYS_wait_s:
			break;
		case SYS_wait_t:
			break;
		case SYS_signal:
			break;
		case SYS_readdir:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			if(syscall_get_int(&int_arg2, esp, 2)) break;
			if(syscall_get_buffer(str_arg1, 
				sizeof(struct directent),
				SYSCALL_STRLEN, esp, 3)) break;
			if(syscall_get_int(&int_arg3, esp, 3)) break;
			return_value = sys_readdir(int_arg1, int_arg2,
				(struct directent*)int_arg3);
			break;
	}
	
	return return_value; /* Syscall successfully handled. */
}

void trap_return(void);
int sys_fork(void)
{
  struct proc* new_proc = alloc_proc();
  if(!new_proc) return -1;
  slock_acquire(&ptable_lock);
  new_proc->k_stack = (uchar*) palloc() + PGSIZE;
  new_proc->pid = next_pid++;
  new_proc->uid = rproc->uid;
  new_proc->gid = rproc->gid;
  new_proc->parent = rproc;
  new_proc->heap_start = rproc->heap_start;
  new_proc->heap_end = rproc->heap_end;
  new_proc->stack_start = rproc->stack_start;
  new_proc->stack_end = rproc->stack_end;
  new_proc->state = PROC_RUNNABLE;
  new_proc->pgdir = (uint*) palloc();
  vm_copy_kvm(new_proc->pgdir);
  vm_copy_uvm(new_proc->pgdir, rproc->pgdir); 
  memmove(new_proc->k_stack - PGSIZE, rproc->k_stack - PGSIZE, PGSIZE);
  new_proc->tf = (struct trap_frame*)((char*)new_proc->k_stack - 
		sizeof(struct trap_frame));
  new_proc->tf->eax = 0; /* The child should return 0 */
  new_proc->t = rproc->t;
  new_proc->entry_point = rproc->entry_point;
  new_proc->tss = (struct task_segment*)(new_proc->k_stack - PGSIZE);

  /* Create a context to be restored, trap return should be called. */
  struct context* new_context = (struct context*)((char*)new_proc->tf - 
		sizeof(struct context));
  new_context->cr0 = PGROUNDDOWN((uint)new_proc->pgdir);
  new_context->esp = (uint)new_proc->tf - 4;
  new_context->ebp = new_context->esp;
  new_context->eip = (uint)trap_return;
  new_proc->context = (uint)new_context;

  /* Copy all file descriptors */
  memmove(&new_proc->file_descriptors, &rproc->file_descriptors,
		sizeof(struct file_descriptor) * MAX_FILES);

  memmove(&new_proc->cwd, &rproc->cwd, MAX_PATH_LEN);
  memmove(&new_proc->name, &rproc->name, MAX_PROC_NAME);
  slock_release(&ptable_lock);

  return new_proc->pid;
}

int sys_wait(int pid)
{
  slock_acquire(&ptable_lock);

  int ret_pid = 0;
  struct proc* p = NULL;
  while(1)
  {
    int process;
    for(process = 0;process < PTABLE_SIZE;process++)
    {
      if(ptable[process].state == PROC_KILLED
        && ptable[process].parent == rproc)
      {
        if(pid == -1 || ptable[process].pid == pid)
	{
          p = ptable + process;
          break;
        }
      }
    }

    if(p)
    {
      /* Harvest the child */
      ret_pid = p->pid;
      /* Free used memory */
      freepgdir(p->pgdir);
      /* Free kernel stack */
      pfree((uint)p->k_stack);
      
      memset(p, 0, sizeof(struct proc));
      p->state = PROC_UNUSED;
      break;
    } else {
      /* Lets block ourself */
      rproc->block_type = PROC_BLOCKED_WAIT;
      rproc->b_pid = pid;
      rproc->state = PROC_BLOCKED;
      /* Wait for a signal. */
      yield_withlock();
      /* Reacquire ptable lock */
      slock_acquire(&ptable_lock);
    }
  }

  /* Release the ptable lock */
  slock_release(&ptable_lock);  

  return ret_pid;
}

int sys_exec(const char* path, const char** argv)
{
  /* Create a copy of the path */
  char program_path[MAX_PATH_LEN];
  memset(program_path, 0, MAX_PATH_LEN);
  strncpy(program_path, path, MAX_PATH_LEN);
  /* acquire ptable lock */
  slock_acquire(&ptable_lock);

  /* Create argument array */
  char* args[MAX_ARG];
  memset(args, 0, MAX_ARG * sizeof(char*));

  /* Create a temporary stack: */
  uchar* tmp_stack = (uchar*)palloc() + PGSIZE;
  uint stack_start = (uint)tmp_stack - PGSIZE;
  uint uvm_stack = KVM_START;

  /* copy arguments */
  int x;
  for(x = 0;argv[x];x++)
  {
    uint str_len = strlen(argv[x]) + 1;
    tmp_stack -= str_len;
    uvm_stack -= str_len;
    memmove(tmp_stack, (char*)argv[x], str_len);
    args[x] = (char*)uvm_stack;
  }

  /* Copy argument array */
  tmp_stack -= MAX_ARG * sizeof(char*);
  uvm_stack -= MAX_ARG * sizeof(char*);
  memmove(tmp_stack, args, MAX_ARG * sizeof(char*));
  uint arg_arr_ptr = uvm_stack;
  int arg_count = x;

  /* Push argv */
  tmp_stack -= 4;
  uvm_stack -= 4;
  *((uint*)tmp_stack) = arg_arr_ptr;

  /* push argc */
  tmp_stack -= 4;
  uvm_stack -= 4;
  *((uint*)tmp_stack) = (uint)arg_count;
  
  /* Add bogus return address */
  tmp_stack -= 4;
  uvm_stack -= 4;
  *((uint*)tmp_stack) = 0xFFFFFFFF;
  
  /* Free user memory */
  vm_free_uvm(rproc->pgdir);

  /* Invalidate the TLB */
  switch_uvm(rproc);

  /* load the binary if possible. */
  uint load_result = load_binary(program_path, rproc);
  if(load_result == 0) 
  {
    pfree(stack_start);
    return -1;
  }

  /* Map the new stack in */
  mappage(stack_start, KVM_START - PGSIZE, rproc->pgdir, 1, 0);

  /* We now have the esp and ebp. */
  rproc->tf->esp = uvm_stack;
  rproc->tf->ebp = rproc->tf->esp;

  /* Set eip to correct entry point */
  rproc->tf->eip = rproc->entry_point;

  /* Adjust heap start and end */
  rproc->heap_start = PGROUNDDOWN(load_result + PGSIZE - 1);
  rproc->heap_end = 0;

  /* Release the ptable lock */
  slock_release(&ptable_lock);

  return 0;
}

void sys_exit(void)
{
  /* Acquire the ptable lock */
  slock_acquire(&ptable_lock);

  /* Close all open files */
  int i;
  for(i = 0; i < MAX_FILES; i++){
    sys_close(i);
  }

  /* Set state to killed */
  rproc->state = PROC_KILLED;

  /* Attempt to wakeup our parent */
  if(rproc->zombie == 0)
  {
    if(rproc->parent->block_type == PROC_BLOCKED_WAIT)
    {
      if(rproc->parent->b_pid == -1 || rproc->parent->b_pid == rproc->pid)
      {
        /* Our parent is waiting on us, release the block. */
        rproc->parent->block_type = PROC_BLOCKED_NONE;
        rproc->parent->state = PROC_RUNNABLE;
        rproc->parent->b_pid = 0;
      }
    }
  }

  /* Release ourself to the scheduler, never to return. */
  yield_withlock();
}

int sys_open(const char* path, int flags, int permissions)
{
  int i;
  int fd = -1;
  for(i = 0; i < MAX_FILES; i++){
    if(rproc->file_descriptors[i].type == 0x00){
      fd = i;
      break;
    }
  } 
  if(fd == -1){
    return -1;
  }
  rproc->file_descriptors[fd].type = FD_TYPE_FILE;
  rproc->file_descriptors[fd].flags = flags;
  rproc->file_descriptors[fd].i = fs_open((char*) path, flags, 
		permissions, rproc->uid, rproc->uid);
  if(rproc->file_descriptors[fd].i == NULL){
    memset(rproc->file_descriptors + fd, 0, sizeof(struct file_descriptor));
    return -1;
  }
  rproc->file_descriptors[fd].seek = 0;
  return fd;
}

int sys_close(int fd)
{
  if(fd >= MAX_FILES){return -1;}
  rproc->file_descriptors[fd].type = 0x00;  
  return 0;
}

int sys_read(int fd, char* dst, uint sz)
{
  int i;
  switch(rproc->file_descriptors[fd].type)
  {
    case 0x00: return -1;
    case FD_TYPE_FILE:
      if(fs_read(rproc->file_descriptors[fd].i, dst, sz,
        rproc->file_descriptors[fd].seek) < 0) {
          return -1;
      }
      break;
    case FD_TYPE_DEVICE:
      dev_read(rproc->file_descriptors[fd].device, dst,
		rproc->file_descriptors[fd].seek, sz);
      break;
    case FD_TYPE_STDIN:
      for(i = 0;i < sz; i++){
        char next_char = tty_get_char(rproc->t);
        dst[i] = next_char;
      }
      break;
    default: return -1;
  } 

  rproc->file_descriptors[fd].seek += sz;
  return sz;
}

int sys_write(int fd, char* src, uint sz)
{
  if(rproc->file_descriptors[fd].type == 0x00){
    return -1;
  }  
  else if(rproc->file_descriptors[fd].type == FD_TYPE_FILE){
    if(fs_write(rproc->file_descriptors[fd].i, src, sz, 
	rproc->file_descriptors[fd].seek) == -1){
      return -1;
    }
  }
  else{
    int x;
    int str_len = strlen(src);
    for(x = 0;x < str_len;x++)
      tty_print_character(rproc->t, src[x]);
  }
  rproc->file_descriptors[fd].seek += sz;
  return sz;
}

int sys_lseek(int fd, int offset, int whence)
{
  if(rproc->file_descriptors[fd].type != FD_TYPE_FILE){
    return -1;
  }
  int seek_pos;
  if(whence == SEEK_CUR){
    seek_pos = rproc->file_descriptors[fd].seek + offset;
  }  
  else if(whence == SEEK_SET){
    seek_pos = offset;
  }
  else if(whence == SEEK_END){
    struct file_stat stat;
    fs_stat(rproc->file_descriptors[fd].i, &stat);
    seek_pos = stat.sz + offset;
  } else {
    return -1;
  }
  if(seek_pos < 0){ seek_pos = 0;}
  rproc->file_descriptors[fd].seek = seek_pos;
  return seek_pos;
}

void* sys_mmap(void* hint, uint sz, int protection)
{
  uint pagestart=PGROUNDDOWN(rproc->heap_start + rproc->heap_end + PGSIZE - 1);
  mappages(pagestart, sz, rproc->pgdir, 1);
             
  uint* returnpage = (uint*) pagestart;
  return returnpage;
}

int sys_wait_s(struct cond* c, struct slock* lock)
{	
	slock_acquire(&ptable_lock);
	if(c->next_signal>c->current_signal){
		c->next_signal = 0;
		c->current_signal = 0;
	}
	rproc->block_type = PROC_BLOCKED_COND;
	rproc->b_condition = c;
	rproc->state = PROC_BLOCKED;
	rproc->b_condition_signal = c->current_signal++;
	slock_release(lock);
	slock_release(&ptable_lock);
	return 0;
}

int sys_wait_t(struct cond* c, struct tlock* lock)
{
	slock_acquire(&ptable_lock);
	if(c->next_signal>c->current_signal){
		c->next_signal = 0;
		c->current_signal = 0;
	}
	rproc->block_type = PROC_BLOCKED_COND;
	rproc->b_condition = c;
	rproc->state = PROC_BLOCKED;
	rproc->b_condition_signal = c->current_signal++;
	tlock_release(lock);
	slock_release(&ptable_lock);
	return 0;
}

int sys_signal(struct cond* c)
{
	slock_acquire(&ptable_lock);
	int i;
	for(i = 0; i< PTABLE_SIZE; i++){
		if(ptable[i].state != PROC_BLOCKED){
			continue;
		}
		if(ptable[i].block_type != PROC_BLOCKED_COND){
			continue;
		}
		if(ptable[i].b_condition != c){
			continue;
		}
		if(c->next_signal!=ptable[i].b_condition_signal){
			continue;
		}
		ptable[i].state = PROC_RUNNABLE;
		ptable[i].block_type = PROC_BLOCKED_NONE;
		c->next_signal++;
		break;
	}
	
	slock_release(&ptable_lock);
	return 0;
}

int sys_chdir(const char* dir){

  //strncpy(rproc->cwd, dir, sz);
  return 0;
}

int sys_cwd(char* dst, uint sz){
  strncpy(dst, rproc->cwd  , sz);
  return sz;
}

int sys_create(const char* file, uint permissions){ 
  fs_create(file, 0, permissions, rproc->uid, rproc->uid);  
  return 0;
}

int sys_mkdir(const char* dir, uint permissions){
  fs_mkdir(dir, 0, permissions, rproc->uid, rproc->uid);
  return 0;
}

int sys_rm(const char* file){
  return 0;
}

int sys_rmdir(const char* dir){
  return 0;
}

int sys_mv(const char* orig, const char* dst){
  return 0;
}

int sys_fstat(int fd, struct file_stat* dst)
{
  if(rproc->file_descriptors[fd].type != FD_TYPE_FILE)
	return -1;
  return fs_stat(rproc->file_descriptors[fd].i, dst);
}

int sys_readdir(int fd, int index, struct directent* dst)
{
	if(rproc->file_descriptors[fd].type != FD_TYPE_FILE)
		return -1;
	return fs_readdir(rproc->file_descriptors[fd].i, index, dst);
}
