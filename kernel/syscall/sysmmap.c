#include "stdlock.h"
#include "file.h"
#include "syscall.h"
#include "pipe.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "proc.h"
#include "vm.h"
#include "chronos.h"
#include "panic.h"


void* mmap(void* hint, size_t sz, int protection,
        int flags, int fd, off_t offset);

static void* mmap_find_space(size_t sz)
{
	/* Lets start searching from the start */
	int pages_needed = PGROUNDUP(sz) / PGSIZE;
	int page = PGROUNDDOWN(rproc->mmap_start) - PGSIZE;
	int pages = 0;
	void* address = NULL;
	for(;page > rproc->heap_end;page -= PGSIZE)
	{
		if(vm_findpg(page, 0, rproc->pgdir, 0, 0))
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
	uintptr_t address = (uintptr_t)addr;
	if(length == 0) return -1;
	/* Address must be page aligned */
	if(address != PGROUNDDOWN(address)) return -1;

	/* Address must be in the proper range */
	if(address < PGROUNDUP(rproc->heap_end) + PGSIZE ||
			address >= PGROUNDDOWN(rproc->mmap_start) ||
			address + length > PGROUNDDOWN(rproc->mmap_start))
		return -1;

	uintptr_t x;
	for(x = 0;x < length;x += PGSIZE)
		vm_unmappage(x + address, rproc->pgdir);

	return 0;
}



/* void* mmap(void* hint, size_t sz, int protection,
   int flags, int fd, off_t offset) */
int sys_mmap(void)
{
        void* hint;
        size_t sz;
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
void* mmap(void* hint, size_t sz, int protection, 
	int flags, int fd, off_t offset)
{
	/* Acquire locks */
	vmpage_t pagestart = 0;
	int hint_okay = 1;
	if((uintptr_t)hint != PGROUNDDOWN((uintptr_t)hint) || !hint)
		hint_okay = 0;

	/* If a hint is provided and its not okay, return NULL */
	if(hint && !hint_okay)
		return NULL;

	/* acquire the memory lock */	
	slock_acquire(&rproc->mem_lock);

	if(hint_okay && (flags & MAP_FIXED))
	{
		vmpage_t addr = PGROUNDDOWN((vmpage_t)hint);
		/* Is the address appropriate? */
		if(addr >= PGROUNDUP(rproc->heap_end) + PGSIZE && 
				addr < rproc->mmap_start && 
				addr + sz < rproc->mmap_start)
			pagestart = addr;
		else {
			slock_release(&rproc->mem_lock);
			return NULL;
		}
	} else {
        	pagestart = (uintptr_t)mmap_find_space(sz);
	}

	if(!pagestart) 
	{
		slock_release(&rproc->mem_lock);
		return NULL;
	}

	if(pagestart < rproc->mmap_end)
		rproc->mmap_end = pagestart;

	vmflags_t dir_flags = VM_DIR_USRP | VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = 0;
	if(protection & PROT_WRITE)
		tbl_flags |= VM_TBL_WRIT;
	if(protection & PROT_EXEC)
		tbl_flags |= VM_TBL_EXEC;
	if(protection & PROT_READ)
		tbl_flags |= VM_TBL_READ;
	
	if(!protection)
	{
		slock_release(&rproc->mem_lock);
		return NULL;
	}

	/* Enable default flags */
	tbl_flags |= VM_TBL_USRP | VM_TBL_READ;

        vm_mappages(pagestart, sz, rproc->pgdir, dir_flags, tbl_flags);

#ifdef  __ALLOW_VM_SHARE__
	/* Is this mapping shared? */
	if(flags & MAP_SHARED)
	{
		if(vm_pgsshare(pagestart, sz, rproc->pgdir))
		{
			slock_release(&rproc->mem_lock);
			return NULL;
		}
	}
#endif

	/* Release locks */
	slock_release(&rproc->mem_lock);
        return (void*)pagestart;
}
