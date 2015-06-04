#include "types.h"
#include "elf.h"
#include "vsfs.h"
#include "tty.h"
#include "stdlock.h"
#include "proc.h"
#include "vm.h"
#include "trap.h"
#include "panic.h"
#include "stdarg.h"
#include "stdlib.h"
#include "x86.h"
#include "syscall.h"

/* The process table lock must be acquired before accessing the ptable. */
slock_t ptable_lock;
/* The process table */
struct proc ptable[PTABLE_SIZE];
/* A pointer into the ptable of the running process */
struct proc* rproc;
/* The next available pid */
uint next_pid;

void proc_init()
{
	next_pid = 0;
	memset(ptable, 0, sizeof(struct proc) * PTABLE_SIZE);
	rproc = NULL;
	slock_acquire(&ptable_lock);
}

struct proc* alloc_proc()
{
	slock_acquire(&ptable_lock);
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_UNUSED)
		{
			ptable[x].state = PROC_EMBRYO;
			break;
		}
	}

	slock_release(&ptable_lock);

	if(x >= PTABLE_SIZE) return NULL;
	return ptable + x;
}

struct proc* spawn_tty(tty_t t)
{
	/* Try to get a new process */
	struct proc* p = alloc_proc();
	if(!p) return NULL; /* Could we find an unused process? */

	/* Get the process table lock */
	slock_acquire(&ptable_lock);

	/* Setup the new process */
	p->t = t;
	p->pid = next_pid++;
	p->uid = 0; /* init is owned by root */
	p->gid = 0; /* group is also root */
	memset(p->file_descriptors, 0, 
		sizeof(struct file_descriptor) * MAX_FILES);

	p->stack_start = KVM_START;
	p->stack_end = KVM_START - PGSIZE;
	
	p->heap_start = 0;
	p->heap_end = 0;

	p->block_type = PROC_BLOCKED_NONE;
	p->b_condition = NULL;
	p->b_pid = 0;
	p->parent = p;

	strncpy(p->name, "init", MAX_PROC_NAME);
	strncpy(p->cwd, "/", MAX_PATH_LEN);

	/* Setup virtual memory */
	p->pgdir = (pgdir*)palloc();
	vm_copy_kvm(p->pgdir);

	/* Setup a kernel stack */
	p->k_stack = (uchar*)(palloc() + PGSIZE);
	p->tf = (struct trap_frame*)(p->k_stack - sizeof(struct trap_frame));
	p->tss = p->k_stack - PGSIZE;

	/* Map in a user stack. */
	mappages(KVM_START - PGSIZE, PGSIZE, p->pgdir, 1);

	/* Load the binary */
	load_binary("/bin/init", p);

	p->state = PROC_READY;
	slock_release(&ptable_lock);

	return p;
}

void load_binary(const char* path, struct proc* p)
{
	vsfs_inode process_file;
	int inonum;
        if((inonum = vsfs_lookup(path, &process_file)) == 0)
                panic("Cannot find process executable.");

	        /* Sniff to see if it looks right. */
        uchar elf_buffer[4];
        vsfs_read(&process_file, 0, 4, elf_buffer);
        char elf_buff[] = ELF_MAGIC;
        if(memcmp(elf_buffer, elf_buff, 4))
                panic("Elf magic is wrong");

	/* Load the entire elf header. */
        struct elf32_header elf;
        vsfs_read(&process_file, 0, sizeof(struct elf32_header), &elf);
        /* Check class */
        if(elf.exe_class != 1) panic("Binary not executable");
        if(elf.version != 1) panic("Binary wrong ELF version");
        if(elf.e_type != ELF_E_TYPE_EXECUTABLE) panic("Binary wrong exe type");
        if(elf.e_machine != ELF_E_MACHINE_x86) panic("Binary wrong ISA");
        if(elf.e_version != 1) panic("Binary wrong machine version");

        uint elf_entry = elf.e_entry;

	int x;
        for(x = 0;x < elf.e_phnum;x++)
        {
                int header_loc = elf.e_phoff + (x * elf.e_phentsize);
                struct elf32_program_header curr_header;
                vsfs_read(&process_file, header_loc,
                        sizeof(struct elf32_program_header),
                        &curr_header);
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
			/* Paging: allocate user pages */
			mappages((uint)hd_addr, mem_sz, p->pgdir, 1);
                        /* zero this region */
                        memset(hd_addr, 0, mem_sz);
                        /* Load the section */
                        vsfs_read(&process_file, offset,
                                file_sz, hd_addr);
                        /* By default, this section is rwx. */
                }
        }

	/* Set the entry point of the program */
	p->entry_point = elf_entry;
}

void sched_init()
{
	/* Zero all of the processes (unused) */
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
		memset(ptable + x, 0, sizeof(struct proc));
	/* No process is running right now. */
	rproc = NULL;
	/* Initilize our process table lock */
	slock_init(&ptable_lock);
}

struct proc* get_proc_pid(int pid)
{
	int x;
        for(x = 0;x < PTABLE_SIZE;x++)
        {
		if(ptable[x].pid == pid)
			return ptable + x;
	}

	/* There is no process with that pid. */
	return NULL;
}

void sched(void)
{
	slock_acquire(&ptable_lock);
	scheduler();
}

void scheduler(void)
{
  slock_acquire(&ptable_lock);
  
  int i;
  int curr_index;
  for(i = 0; i < PTABLE_SIZE; i++){
    if(ptable + i == rproc){
      curr_index = i;
    }
  }  
  curr_index++;
  while(ptable[curr_index].state != PROC_RUNNABLE){
    if(curr_index == PTABLE_SIZE - 1){
      curr_index = 0;
    }
    curr_index++;
  } 
  rproc = &ptable[curr_index];	
  switch_uvm(rproc);
}
