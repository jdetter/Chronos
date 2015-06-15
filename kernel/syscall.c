#include "types.h"
#include "x86.h"
#include "vsfs.h"
#include "tty.h"
#include "stdlock.h"
#include "proc.h"
#include "vm.h"
#include "file.h"
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
extern struct proc* ptable; /* The process table */
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
			break;
		case SYS_open:
			break;
		case SYS_close:
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
			break;
		case SYS_wait_s:
			break;
		case SYS_wait_t:
			break;
		case SYS_signal:
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
  rproc->state = PROC_BLOCKED;
  rproc->b_pid = pid;
  scheduler();
  return 0;
}

int sys_exec(const char* path, const char** argv)
{
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
  
  /* load the binary if possible. */
  uint load_result = load_binary(path, rproc);
  if(load_result == 0) 
  {
    pfree(stack_start);
    return -1;
  }
  
  /* New stack is complete, free old stack. */
  uint old_stack = findpg(KVM_START - PGSIZE, 0, rproc->pgdir, 0);
  unmappage(KVM_START - PGSIZE, rproc->pgdir);
  pfree(old_stack);

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

  /* Make sure that the heap has room to grow */
  for(x = rproc->heap_start;x < KVM_START - PGSIZE;x++)
  {
    uint page = unmappage(x, rproc->pgdir);
    if(!page) break;
    pfree(page);
  }

  /* Release the ptable lock */
  slock_release(&ptable_lock);

  return 0;
}

void sys_exit(void)
{
  int i;
  for(i = 0; i < MAX_FILES; i++){
    if(rproc->file_descriptors[i].type != 0x00){
      sys_close(i);
    }  
  }
  rproc->state = PROC_KILLED; 
  sched();       	
}

int sys_open(const char* path)
{
  int i;
  int fd = -1;
  for(i = 0; i < MAX_FILES; i++){
    if(rproc->file_descriptors[i].type == 0x00){
      fd = i;
    }
  } 
  if(fd == -1){
    return -1;
  }
  rproc->file_descriptors[fd].type = FD_TYPE_FILE;
  int inode = vsfs_lookup((char*) path, &rproc->file_descriptors[fd].inode);
  if(inode == 0){
    return -1;
  }
  rproc->file_descriptors[fd].inode_pos = 0;
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
  if(rproc->file_descriptors[fd].type == 0x00){
    return -1;
  }  
  
  if(rproc->file_descriptors[fd].type == FD_TYPE_FILE){
    if(vsfs_read(&rproc->file_descriptors[fd].inode,
      rproc->file_descriptors[fd].inode_pos, sz, dst) == -1) {
      return -1;
    }
  } else {
    int i;
    for(i = 0;i < sz; i++){
      char next_char = tty_get_char(rproc->t);
      dst[i] = next_char;
    } 
  }
  rproc->file_descriptors[fd].inode_pos += sz;
  return sz;
}

int sys_write(int fd, char* src, uint sz)
{
  if(rproc->file_descriptors[fd].type == 0x00){
    return -1;
  }  
  else if(rproc->file_descriptors[fd].type == FD_TYPE_FILE){
    if(vsfs_write(&rproc->file_descriptors[fd].inode,
      rproc->file_descriptors[fd].inode_pos, sz, src) == -1){
      return -1;
    }
  }
  else{
    tty_print_string(rproc->t, src);
  }
  rproc->file_descriptors[fd].inode_pos += sz;
  return sz;
}

int sys_lseek(int fd, int offset, int whence)
{
  if(rproc->file_descriptors[fd].type != FD_TYPE_FILE){
    return -1;
  }
  int seek_pos;
  if(whence == FSEEK_CUR){
    seek_pos = rproc->file_descriptors[fd].inode_pos + offset;
  }  
  else if(whence == FSEEK_SET){
    seek_pos = offset;
  }
  else{
    seek_pos = rproc->file_descriptors[fd].inode.size + offset;
  }
  if(seek_pos < 0){ seek_pos = 0;}
  rproc->file_descriptors[fd].inode_pos = seek_pos;
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

int sys_chdir(const char* dir, uint sz){

  strncpy(rproc->cwd, dir, sz);
  return 0;
}

int sys_cwd(char* dst, uint sz){
  strncpy(dst, rproc->cwd  , sz);
  return sz;
}

int sys_create(const char* file, uint permissions){
  
  struct vsfs_inode new_inode;
  new_inode.perm = permissions; 
  vsfs_link(file, &new_inode);  
  return 0;
}

int mkdir(const char* dir, uint permissions){

  struct vsfs_inode new_inode;
  new_inode.perm = permissions;
  vsfs_link(dir, &new_inode);
  return 0;

}

int rm(const char* file){

  vsfs_inode toremove;
  int inode_num = vsfs_lookup(file, &toremove);
  if(inode_num == 0){ return -1;}
  if(toremove.type != VSFS_FILE){
    return -1;
  } 
  vsfs_unlink(file);
  return 0;
}

int rmdir(const char* dir){

  vsfs_inode toremove;
  int inode_num = vsfs_lookup(dir, &toremove);
  if(inode_num == 0){ return -1;}
  if(toremove.type != VSFS_DIR){
    return -1;
  } 
  if(toremove.size > (sizeof(directent)*2)){
    return -1;
  }
  vsfs_unlink(dir);
  return 0;
}

int mv(const char* orig, const char* dst){

  vsfs_inode tomove;
  int inode_num = vsfs_lookup(orig, &tomove);
  if(inode_num == 0){ return -1;}
  if(tomove.type != VSFS_FILE){
    return -1;
  }
  vsfs_link(dst, &tomove);
  vsfs_unlink(orig);
  return 0;
}

int fstat(const char* path, struct stat* dst){

  vsfs_inode tostat ;
  int inode_num = vsfs_lookup(path, &tostat);
  if(inode_num == 0){ return -1;}
  if(tostat.type != VSFS_FILE){
    return -1;
  }
  dst->owner_id = tostat.uid; /* Owner of the file */
  dst->group_id = tostat.gid; /* Group that owns the file */
  dst->perm = tostat.perm; /* Permissions of the file */
  dst->sz = tostat.size; /* Size of the file in bytes */
  dst->links = tostat.links_count; /* The amount of hard links to this file */
  dst->type = tostat.type; /* See options for file type above*/ 
  return 0;
}
