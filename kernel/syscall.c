#include "types.h"
#include "chronos.h"
#include "stdlock.h"
#include "vsfs.h"
#include "tty.h"
#include "proc.h"
#include "elf.h"
#include "stdlib.h"

extern slock_t ptable_lock; /* Process table lock */
extern struct proc* ptable; /* The process table */
extern struct proc* rproc; /* The currently running process */

int sys_close(int fd);

int sys_fork(void)
{ 
	return 0;
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
  slock_acquire(&ptable_lock);
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
      /* zero this region */
      memset(hd_addr, 0, mem_sz);
      /* Load the section */
      vsfs_read(&to_exec, offset, file_sz, hd_addr);
      /* By default, this section is rwx. */
    }
  }
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

int fseek(int fd, int offset, int whence)
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
