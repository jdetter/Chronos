#include "types.h"
#include "x86.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "pipe.h"
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
	if(syscall_addr_safe(str) || syscall_addr_safe(str + sz_user - 1))
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
        if(syscall_addr_safe(buff) || syscall_addr_safe(buff + sz - 1))
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

/**
 * Find an open file descripor.
 */
static int find_fd(void)
{
	int x;
	for(x = 0;x < MAX_FILES;x++)
		if(rproc->file_descriptors[x].type == 0x0)
			return x;
	return -1;
}

int syscall_handler(uint* esp)
{
	/* Get the number of the system call */
	int syscall_number = -1;
	int return_value = -1;
	if(syscall_get_int(&syscall_number, esp, 0)) return 1;
	//cprintf("Syscall number: %d\n", syscall_number);
	esp++;
	int int_arg1;
	int int_arg2;
	int int_arg3;

#define SYSCALL_STRLEN 512
	char str_arg1[SYSCALL_STRLEN];
	//char str_arg2[SYSCALL_STRLEN];

	void* ptr_arg1;
	void* ptr_arg2;

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
			if(syscall_get_str((char*)str_arg1,
                                SYSCALL_STRLEN, esp, 1)) break;
			return_value = sys_chdir(str_arg1);
			break;
		case SYS_cwd:
			if(syscall_get_int(&int_arg1, esp, 2)) break;
			if(syscall_get_buffer_ptr((char**)&ptr_arg1,
                                int_arg1, esp, 1)) break;
			return_value = sys_cwd(ptr_arg1, int_arg1);
			break;
		case SYS_create:
			if(syscall_get_str((char*)str_arg1,
                                SYSCALL_STRLEN, esp, 1)) break;
                        if(syscall_get_int(&int_arg1, esp, 2)) break;
			return_value = sys_create(str_arg1, int_arg1);
			break;
		case SYS_mkdir:
			break;
		case SYS_rmdir:
			break;
		case SYS_rm:
			if(syscall_get_str((char*)str_arg1,
				SYSCALL_STRLEN, esp, 1)) break;
			return_value = sys_rm(str_arg1);
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
			if(syscall_get_buffer_ptr((char**)&ptr_arg1, 
				sizeof(struct cond), esp, 1)) break;
			if(syscall_get_buffer_ptr((char**)&ptr_arg2, 
				sizeof(struct slock), esp, 2)) break;
			return_value = sys_wait_s(ptr_arg1, ptr_arg2);
			break;
		case SYS_wait_t:
                        if(syscall_get_buffer_ptr((char**)&ptr_arg1, 
                                sizeof(struct cond), esp, 1)) break;
                        if(syscall_get_buffer_ptr((char**)&ptr_arg2,
                                sizeof(struct slock), esp, 2)) break;
                        return_value = sys_wait_t(ptr_arg1, ptr_arg2);
			break;
		case SYS_signal:
                        if(syscall_get_buffer_ptr((char**)&ptr_arg1, 
                                sizeof(struct cond), esp, 1)) break;
                        return_value = sys_signal(ptr_arg1);
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
		case SYS_pipe:
			if(syscall_get_buffer_ptr((char**)&ptr_arg1, 
				2 * sizeof(int),
                                esp, 1)) break;
			return_value = sys_pipe(ptr_arg1);
			break;
		case SYS_dup:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			return_value = sys_dup(int_arg1);
			break;
		case SYS_dup2:
			if(syscall_get_int(&int_arg1, esp, 1)) break;
			if(syscall_get_int(&int_arg2, esp, 2)) break;
			return_value = sys_dup2(int_arg1, int_arg2);
			break;
		case SYS_proc_dump:
			return_value = sys_proc_dump();
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
  new_proc->k_stack = rproc->k_stack;
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
  new_proc->tf = rproc->tf;
  new_proc->t = rproc->t;
  new_proc->entry_point = rproc->entry_point;
  new_proc->tss = rproc->tss;
  new_proc->context = rproc->context;
  memmove(&new_proc->cwd, &rproc->cwd, MAX_PATH_LEN);
  memmove(&new_proc->name, &rproc->name, MAX_PROC_NAME);

  /* Copy all file descriptors */
  memmove(&new_proc->file_descriptors, &rproc->file_descriptors,
                sizeof(struct file_descriptor) * MAX_FILES);

  /* Increment file references for all inodes */
  int i;
  for(i = 0;i < MAX_FILES;i++)
    if(new_proc->file_descriptors[i].type == FD_TYPE_FILE)
    {
      fs_add_inode_reference(new_proc->file_descriptors[i].i);
    }

  /** 
   * A quick note on the swap stack:
   *
   * The swap stack is an area in memory located just below the kernel
   * stack for the running program. This are is used to map in another
   * stack from another program. This is useful when you want to work
   * on 2 stacks at the same time without changing page directories.
   * Here we are using the swap stack to map in the stack of our new
   * process and modifying the contents without changing page directories.
   */

  /* Map the new process's stack into our swap space */
  vm_set_swap_stack(rproc->pgdir, new_proc->pgdir);

  struct trap_frame* tf = (struct trap_frame*)
		((uchar*)new_proc->tf - SVM_DISTANCE);
  tf->eax = 0; /* The child should return 0 */
  
  /* new proc needs a context */
  struct context* c = (struct context*)
		((uchar*)tf - sizeof(struct context));
  c->eip = (uint)trap_return;
  c->esp = (uint)tf - 4 + SVM_DISTANCE;
  c->cr0 = (uint)new_proc->pgdir;
  new_proc->context = (uint)c + SVM_DISTANCE;

  /* Clear the swap stack now */
  vm_clear_swap_stack(rproc->pgdir);

  /* release ptable lock */
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
    
      /* change rproc to the child process so that we can close its files. */
      struct proc* current = rproc;
      rproc = p; 
      /* Close open files */
      int file;
      for(file = 0;file < MAX_FILES;file++)
        sys_close(file);
      rproc = current;

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
  char cwd_tmp[MAX_PATH_LEN];
  memmove(cwd_tmp, rproc->cwd, MAX_PATH_LEN);
  /* Does the binary look ok? */
  if(check_binary(path))
  {
    /* see if it is available in /bin */
    strncpy(rproc->cwd, "/bin/", MAX_PATH_LEN);
    if(check_binary(path))
    {
      /* It really doesn't exist. */
      memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);
      return -1;
    }
  }

  /* Create a copy of the path */
  char program_path[MAX_PATH_LEN];
  memset(program_path, 0, MAX_PATH_LEN);
  strncpy(program_path, path, MAX_PATH_LEN);
  /* acquire ptable lock */
  slock_acquire(&ptable_lock);

  /* Create argument array */
  char* args[MAX_ARG];
  memset(args, 0, MAX_ARG * sizeof(char*));

  /* Make sure the swap stack is empty */
  vm_clear_swap_stack(rproc->pgdir);

  /* Create a temporary stack and map it to our swap stack */
  uint stack_start = palloc();
  mappage(stack_start, SVM_KSTACK_S, rproc->pgdir, 1, 0);
  uchar* tmp_stack = (uchar*)SVM_KSTACK_S + PGSIZE;
  uint uvm_stack = PGROUNDUP(UVM_USTACK_TOP);

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
    memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);
    slock_release(&ptable_lock);
    return -1;
  }

  /* Change name */
  strncpy(rproc->name, program_path, FILE_MAX_PATH);

  /* Map the new stack in */
  mappage(stack_start, PGROUNDUP(UVM_USTACK_TOP) - PGSIZE, 
		rproc->pgdir, 1, 0);
  /* Unmap the swap stack */
  vm_clear_swap_stack(rproc->pgdir);

  /* We now have the esp and ebp. */
  rproc->tf->esp = uvm_stack;
  rproc->tf->ebp = rproc->tf->esp;

  /* Set eip to correct entry point */
  rproc->tf->eip = rproc->entry_point;

  /* Adjust heap start and end */
  rproc->heap_start = PGROUNDUP(load_result);
  rproc->heap_end = rproc->heap_start;

  /* restore cwd */
  memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);

  /* Release the ptable lock */
  slock_release(&ptable_lock);

  return 0;
}

void sys_exit(void)
{
  /* Acquire the ptable lock */
  slock_acquire(&ptable_lock);

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
  if(fd >= MAX_FILES || fd < 0){return -1;}
  if(rproc->file_descriptors[fd].type == FD_TYPE_FILE)
    fs_close(rproc->file_descriptors[fd].i);
  else if(rproc->file_descriptors[fd].type == FD_TYPE_PIPE)
  {
    /* Do we need to free the pipe? */
    rproc->file_descriptors[fd].pipe->references--;
    if(!rproc->file_descriptors[fd].pipe->references)
      pipe_free(rproc->file_descriptors[fd].pipe);
  }
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
    case FD_TYPE_PIPE:
      if(rproc->file_descriptors[fd].pipe_type == FD_PIPE_MODE_READ)
        pipe_read(dst, sz, rproc->file_descriptors[fd].pipe);
      else return -1;
      break;
    default: return -1;
  } 

  rproc->file_descriptors[fd].seek += sz;
  return sz;
}

int sys_write(int fd, char* src, uint sz)
{
  int x;
  int str_len;
  switch(rproc->file_descriptors[fd].type)
  {
  default: return -1;
  case FD_TYPE_FILE:
    if(fs_write(rproc->file_descriptors[fd].i, src, sz, 
	rproc->file_descriptors[fd].seek) == -1)
      return -1;
    break;
  case FD_TYPE_STDOUT:
    str_len = strlen(src);
    for(x = 0;x < str_len;x++)
      tty_print_character(rproc->t, src[x]);
    break;
  case FD_TYPE_STDERR:
    str_len = strlen(src);
    for(x = 0;x < str_len;x++)
      tty_print_character(rproc->t, src[x]);
    break;
  case FD_TYPE_DEVICE:
    if(rproc->file_descriptors[fd].device->write)
      sz = rproc->file_descriptors[fd].device->write(src, 
           rproc->file_descriptors[fd].seek, sz, 
           rproc->file_descriptors[fd].device->context);
    else return -1;
    break;
  case FD_TYPE_PIPE:
    if(rproc->file_descriptors[fd].pipe_type == FD_PIPE_MODE_WRITE)
      pipe_write(src, sz, rproc->file_descriptors[fd].pipe);
    else return -1;
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
  uint pagestart = PGROUNDUP(rproc->heap_end);
  mappages(pagestart, sz, rproc->pgdir, 1);
  rproc->heap_end += sz;
  return (void*)pagestart;
}

int sys_wait_s(struct cond* c, struct slock* lock)
{	
	slock_acquire(&ptable_lock);
	if(c->next_signal > c->current_signal){
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
		if(ptable[i].state != PROC_BLOCKED) continue;
		if(ptable[i].block_type != PROC_BLOCKED_COND) continue;
		if(ptable[i].b_condition != c) continue;
		if(c->next_signal != ptable[i].b_condition_signal) continue;

		ptable[i].state = PROC_RUNNABLE;
		ptable[i].block_type = PROC_BLOCKED_NONE;
		ptable[i].b_condition_signal = 0;
		c->next_signal++;
		break;
	}
	
	slock_release(&ptable_lock);
	return 0;
}

int sys_chdir(const char* dir){
  if(strlen(dir) < 1) return -1;
  char dir_path[MAX_PATH_LEN];
  char tmp_path[MAX_PATH_LEN];
  memset(dir_path, 0, MAX_PATH_LEN);
  memset(tmp_path, 0, MAX_PATH_LEN);
  if(file_path_dir(dir, dir_path, MAX_PATH_LEN))
    return -1;
  if(fs_path_resolve(dir_path, tmp_path, MAX_PATH_LEN))
    return -1;
  if(strlen(tmp_path) < 1) return -1;
  if(file_path_dir(tmp_path, dir_path, MAX_PATH_LEN))
    return -1;

  /* See if it exists */
  inode i = fs_open(dir_path, O_RDONLY, 0x0, 0x0, 0x0);
  if(i == NULL) return -1;

  /* is it a directory? */
  struct file_stat st;
  fs_stat(i, &st);
  if(st.type != FILE_TYPE_DIR)
  {
    fs_close(i);
    return -1;
  }

  fs_close(i);
  /* Everything is ok. */
  memmove(rproc->cwd, dir_path, MAX_PATH_LEN);
  return 0;
}

int sys_cwd(char* dst, uint sz){
  strncpy(dst, rproc->cwd, sz);
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

int sys_rm(const char* file)
{
  return fs_unlink(file);
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

int sys_pipe(int pipefd[2])
{
	/* Try to get a pipe */
	pipe_t t = pipe_alloc();
	if(!t) return -1;

	/* Get a read fd */
	int read = find_fd();
	if(read >= 0)
	{
		rproc->file_descriptors[read].type = FD_TYPE_PIPE;
		rproc->file_descriptors[read].pipe_type = FD_PIPE_MODE_READ;
		rproc->file_descriptors[read].pipe = t;
	}	

	/* Get a write fd */
	int write = find_fd();
	if(write >= 0)
	{
		rproc->file_descriptors[write].type = FD_TYPE_PIPE;
		rproc->file_descriptors[write].pipe_type = FD_PIPE_MODE_WRITE;
		rproc->file_descriptors[write].pipe = t;
	}

	if(read < 0 || write < 0)
	{
		if(read >= 0)
			rproc->file_descriptors[read].type = 0x0;
		if(write >= 0)
			rproc->file_descriptors[write].type = 0x0;
		pipe_free(t);
		return -1;
	}
	t->references = 2;
	pipefd[0] = read;
	pipefd[1] = write;

	return 0;	
}

int sys_dup(int fd)
{
	int new_fd = find_fd();
	return sys_dup2(new_fd, fd);
}

int sys_dup2(int new_fd, int old_fd)
{
        if(new_fd < 0 || new_fd >= MAX_FILES) return -1;
	if(old_fd < 0 || old_fd >= MAX_FILES) return -1;
	/* Make sure new_fd is closed */
	sys_close(new_fd);
        memmove(rproc->file_descriptors + new_fd,
                rproc->file_descriptors + old_fd,
                sizeof(struct file_descriptor));
        switch(rproc->file_descriptors[old_fd].type)
        {
                default: return -1;
                case FD_TYPE_STDIN:
                case FD_TYPE_STDOUT:
                case FD_TYPE_STDERR:
                        break;
                case FD_TYPE_FILE:
                        rproc->file_descriptors[old_fd].i->references++;
                        break;
                case FD_TYPE_DEVICE:break;
                case FD_TYPE_PIPE:
                        slock_acquire(&rproc->
				file_descriptors[old_fd].pipe->guard);
                        rproc->file_descriptors[old_fd].pipe->references++;
                        slock_release(&rproc->
				file_descriptors[old_fd].pipe->guard);
                        break;
        }

	return new_fd;
}

int sys_proc_dump(void)
{
	tty_print_string(rproc->t, "Running processes:\n");
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		struct proc* p = ptable + x;
		if(p->state == PROC_UNUSED) continue;
		tty_print_string(rproc->t, "++++++++++++++++++++\n");
		tty_print_string(rproc->t, "Name: %s\n", p->name);
		tty_print_string(rproc->t, "DIR: %s\n", p->cwd);
		tty_print_string(rproc->t, "PID: %d\n", p->pid);
		tty_print_string(rproc->t, "UID: %d\n", p->uid);
		tty_print_string(rproc->t, "GID: %d\n", p->gid);
		
		tty_print_string(rproc->t, "state: ");
		switch(p->state)
		{
		case PROC_EMBRYO:	
			tty_print_string(rproc->t, "EMBRYO");
			break;
		case PROC_READY:	
			tty_print_string(rproc->t, "READY TO RUN");
			break;
		case PROC_RUNNABLE:	
			tty_print_string(rproc->t, "RUNNABLE");
			break;
		case PROC_RUNNING:	
			tty_print_string(rproc->t, "RUNNING - "
				"CURRENT PROCESS");
			break;
		case PROC_BLOCKED:	
			tty_print_string(rproc->t, "BLOCKED\n");
			tty_print_string(rproc->t, "Reason: ");
			if(p->block_type == PROC_BLOCKED_WAIT)
			{
				tty_print_string(rproc->t, "CHILD WAIT");
			} else if(p->block_type == PROC_BLOCKED_COND)
			{
				tty_print_string(rproc->t, "CONDITION");
			} else {

				tty_print_string(rproc->t, "INDEFINITLY\n");
			}
			break;
		case PROC_ZOMBIE:	
			tty_print_string(rproc->t, "ZOMBIE");
			break;
		case PROC_KILLED:	
			tty_print_string(rproc->t, "KILLED");
			break;
		default: 
			tty_print_string(rproc->t, "CURRUPT!");
		}
		tty_print_string(rproc->t, "\n");
		
		tty_print_string(rproc->t, "Open Files:\n");
		int file;
		for(file = 0;file < MAX_FILES;file++)
		{
			struct file_descriptor* fd = 
				p->file_descriptors + file;
			if(fd->type == 0x0) continue;
			tty_print_string(rproc->t, "%d: ", file);
			switch(fd->type)
			{
				case FD_TYPE_STDIN:
					tty_print_string(rproc->t, "STDIN");
					break;
				case FD_TYPE_STDOUT:
					tty_print_string(rproc->t, "STDOUT");
					break;
				case FD_TYPE_STDERR:
					tty_print_string(rproc->t, "STDERR");
					break;
				case FD_TYPE_FILE:
					tty_print_string(rproc->t, "FILE");
					break;
				case FD_TYPE_DEVICE:
					tty_print_string(rproc->t, "DEVICE");
					break;
				case FD_TYPE_PIPE:
					tty_print_string(rproc->t, "PIPE");
					break;
				default:
					tty_print_string(rproc->t, 							"CURRUPT");
			}
			tty_print_string(rproc->t, "\n");

		}
		tty_print_string(rproc->t, "\n");

		tty_print_string(rproc->t, "Virtual Memory: ");
		uint stack = p->stack_start - p->stack_end;
		uint heap = p->heap_end - p->heap_start;
		uint code = p->heap_start - 0x1000;
		uint total = (stack + heap + code) / 1024;
		tty_print_string(rproc->t, "%dK\n", total);	
	}

	fs_fsstat();
	return 0;
}
