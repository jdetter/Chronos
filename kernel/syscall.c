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

extern uint next_pid;
extern slock_t ptable_lock; /* Process table lock */
extern struct proc* ptable; /* The process table */
extern struct proc* rproc; /* The currently running process */

/* Is the given address safe to access? */
uchar syscall_addr_safe(uchar* address)
{
	if(rproc->stack_end <= (uint)address 
		&& (uint)address < rproc->stack_start
		&& rproc->heap_end > (uint)address 
		&& (uint)address >= rproc->heap_start)
	{
		/* Invalid memory access */
		return 1;
	}
	/* Valid memory access */
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
int syscall_get_str(char* dst, uint sz, uint* esp, uint arg_num)
{
	esp += arg_num; /* This is a pointer to the string we need */
	uchar* str = (uchar*)esp;
	if(syscall_addr_safe(str) || syscall_addr_safe(str + sz - 1))
		return 1;

	memmove(dst, str, sz);
	return 0;
}

int syscall_handler(uint* esp)
{
	esp += 7;
	/* Get the number of the system call */
	int syscall_number = -1;
	if(syscall_get_int(&syscall_number, esp, 0)) return 1;

	switch(syscall_number)
	{
		case SYS_fork:
			break;
		case SYS_wait:
			break;
		case SYS_exec:
			break;
		case SYS_exit:
			break;
		case SYS_open:
			break;
		case SYS_close:
			break;
		case SYS_read:
			break;
		case SYS_write:
			break;
		case SYS_lseek:
			break;
		case SYS_mmap:
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
	
	return 0; /* Syscall successfully handled. */
}

int sys_fork(void)
{
  struct proc* new_proc = alloc_proc();
  slock_acquire(&ptable_lock);
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
  vm_copy_vm(new_proc->pgdir, rproc->pgdir); 
  new_proc->k_stack = (uchar*) palloc();
  memmove(new_proc->k_stack, rproc->k_stack, PGSIZE);   
  new_proc->tf->eax = 0;

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
  char* args[64];
  memset(args, 0, sizeof(char*) * 64);
  uchar* ogtos = (uchar*)palloc() + PGSIZE; 
  uchar* tos = ogtos;
  tos -= 4;
  int i;
  for(i = 0; argv[i] != NULL; i++){
    tos -= (strlen(argv[i]) + 1);
    args[i] = (char*)tos;
    memmove(tos, args[i], (strlen(argv[i] + 1)));
  }
  tos -= (64 * sizeof(char*)); 
  memmove(tos, args, 64 * sizeof(char*)); 
  tos -= 4;
  uint retadd = 0xFFFFFFFF;
  memmove(tos, &retadd, 4);

  slock_acquire(&ptable_lock);
  freepgdir(rproc->pgdir);
  char* cast_path = (char*) path;

  vsfs_inode to_exec;

  int inode_num;

  inode_num = vsfs_lookup(cast_path, &to_exec);

  if(inode_num == 0){
    return -1;
  }
  if(to_exec.type != VSFS_FILE){
    return -1;
  }

  /* Sniff to see if it looks right. */
  uchar elf_buffer[512];
  vsfs_read(&to_exec, 0, 512, elf_buffer);
  char elf_buff[] = ELF_MAGIC;
  if(memcmp(elf_buffer, elf_buff, 4))
  {
    return -1;
  } 
  /* Load the entire elf header. */
  struct elf32_header elf;
  vsfs_read(&to_exec, 0, sizeof(struct elf32_header), &elf);
  /* Check class */
  if(elf.exe_class != 1) return -1;	
  if(elf.version != 1) return -1;
  if(elf.e_type != ELF_E_TYPE_EXECUTABLE) return -1;	
  if(elf.e_machine != ELF_E_MACHINE_x86) return -1;	
  if(elf.e_version != 1) return -1;
  int x;
  for(x = 0;x < elf.e_phnum;x++)
  {
    int header_loc = elf.e_phoff + (x * elf.e_phentsize);
    struct elf32_program_header curr_header;
    vsfs_read(&to_exec, header_loc, 
      sizeof(struct elf32_program_header), &curr_header);
    /* Skip null program headers */
    if(curr_header.type == ELF_PH_TYPE_NULL) continue;
		
    /* 
     * GNU Stack is a recommendation by the compiler
     * to allow executable stacks. This section doesn't
     * need to be loaded into memory because it's just
     * a flag.
     */
    if(curr_header.type == ELF_PH_TYPE_GNU_STACK)
      continue;
	
    if(curr_header.type == ELF_PH_TYPE_LOAD)
    {
      /* Load this header into memory. */
      uchar* hd_addr = (uchar*)curr_header.virt_addr;
      uint offset = curr_header.offset;
      uint file_sz = curr_header.file_sz;
      uint mem_sz = curr_header.mem_sz;
      if((uint) hd_addr + mem_sz >= KVM_START){
        return -1;
      } 
      mappages((uint) hd_addr, mem_sz, rproc->pgdir, 1);
      /* zero this region */
      memset(hd_addr, 0, mem_sz);
      /* Load the section */
      vsfs_read(&to_exec, offset, file_sz, hd_addr);
      /* By default, this section is rwx. */
    }
  }  
  mappage((uint)ogtos, KVM_START - PGSIZE, rproc->pgdir, 1); 
  scheduler();
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
  else if(rproc->file_descriptors[fd].type == FD_TYPE_FILE){
    if(vsfs_read(&rproc->file_descriptors[fd].inode,
      rproc->file_descriptors[fd].inode_pos, sz, dst) == -1){
      return -1;
    }
  }
  else{
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
