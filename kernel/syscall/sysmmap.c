#include "kern/types.h"
#include "stdlock.h"
#include "file.h"
#include "syscall.h"
#include "pipe.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "proc.h"
#include "x86.h"
#include "vm.h"
#include "chronos.h"
#include "panic.h"

extern slock_t ptable_lock;
extern struct proc* rproc;

void* mmap(void* hint, uint sz, int protection,
        int flags, int fd, off_t offset);

static void* mmap_find_space(uint sz)
{
	/* Lets start searching from the start */
	int pages_needed = PGROUNDUP(sz) / PGSIZE;
	int page = PGROUNDDOWN(rproc->mmap_start) - PGSIZE;
	int pages = 0;
	void* address = NULL;
	for(;page > rproc->heap_end;page -= PGSIZE)
	{
		if(vm_findpg(page, 0, rproc->pgdir, 0))
			pages = 0;
		else {
			pages++;

			if(pages == pages_needed)
			{
				address = (void*)page;
				break;
			}
		}
	}	

	return address;	
}

/* int munmap(void* addr, size_t length) */
int sys_munmap(void)
{
	void* addr;
	size_t length;

	if(syscall_get_int((int*)&addr, 0)) return -1;
        if(syscall_get_int((int*)&length, 1)) return -1;

	/* Is the address okay? */
	uint address = (uint)addr;
	if(length == 0) return -1;
	/* Address must be page aligned */
	if(address != PGROUNDDOWN(address)) return -1;

	/* Address must be in the proper range */
	if(address < PGROUNDUP(rproc->heap_end) + PGSIZE ||
			address >= PGROUNDDOWN(rproc->mmap_start) ||
			address + length > PGROUNDDOWN(rproc->mmap_start))
		return -1;

	uint x;
	for(x = 0;x < length;x += PGSIZE)
		vm_unmappage(x + address, rproc->pgdir);

	return 0;
}



/* void* mmap(void* hint, uint sz, int protection,
   int flags, int fd, off_t offset) */
int sys_mmap(void)
{
        void* hint;
        uint sz;
        int protection;
        int flags;
        int fd;
        off_t offset;

        if(syscall_get_int((int*)&hint, 0)) return -1;
        if(syscall_get_int((int*)&sz, 1)) return -1;
        if(syscall_get_int(&protection, 2)) return -1;
        if(syscall_get_int(&flags,  3)) return -1;
        if(syscall_get_int(&fd, 4)) return -1;
        if(syscall_get_int((int*) &offset, 5)) return -1;

	slock_acquire(&ptable_lock);
	void* ret = mmap(hint, sz, protection, flags, fd, offset);
	slock_release(&ptable_lock);

	return (int)ret;
}

/** MUST HAVE PTABLE LOCK! */
void* mmap(void* hint, uint sz, int protection, 
	int flags, int fd, off_t offset)
{
	/* Acquire locks */
	slock_acquire(&rproc->mem_lock);
	uint pagestart = 0;

	if(flags & MAP_FIXED)
	{
		uint addr = (uint)hint;
		/* Is the address appropriate? */
		if(addr >= PGROUNDUP(rproc->heap_end) + PGSIZE && 
				addr < rproc->mmap_start && 
				addr + sz < rproc->mmap_start)
			pagestart = addr;
	} else {
        	pagestart = (uint)mmap_find_space(sz);
	}

	if(!pagestart) 
	{
		slock_release(&rproc->mem_lock);
        	slock_release(&ptable_lock);
		return 0; /* Not enough space */
	}
	if(pagestart < rproc->mmap_end)
		rproc->mmap_end = pagestart;
        vm_mappages(pagestart, sz, rproc->pgdir, 1);

	/* Check for write protection */
	if(!(protection & PROT_WRITE))
	{
		/* Pages are read only */
		uint x;
		for(x = 0;x < sz;x += PGSIZE)
			pgreadonly(x + pagestart, rproc->pgdir, 1);
	}

	/* Release locks */
	slock_release(&rproc->mem_lock);
        return (void*)pagestart;
}
