#include "vm.h"

/**
 * This generic Copy on Wite (COW) implementation is dependant upon
 * the shared memory implementation. The two work together to provide
 * both shared memory and cow pages.
 */

#if ((!defined _ARCH_SHARE_PROVIDED_) && (defined __ALLOW_VM_SHARE__))

#include "panic.h"

// #define DEBUG

int vm_uvm_cow(pgdir_t* dir)
{
#ifdef DEBUG
	cprintf("cow: Marking page directory 0x%x as cow.\n", dir);
#endif

	vmpage_t p;
	for(p = 0;p < UVM_TOP;p += PGSIZE)
	{
		pypage_t phy = vm_findpg(p, 0, dir, 0, 0);
		/* Does the page exist? */
		if(!phy) continue;
		vmflags_t flags = vm_findtblflags(p, dir);
		/* Is this page already cow? */
		if(flags & VM_TBL_COWR)
		{
#ifdef DEBUG
			cprintf("cow: 0x%x was already cow.", p);
#endif
			/* Add a ref to this page */
			vm_pgshare(p, dir);
			continue;
		}	

		/* This page is not already cow */
		flags |= VM_TBL_COWR;
		if(vm_setpgflags(p, dir, flags))
			return -1;

		/* Create a share for this page */
		if(vm_pgshare(p, dir))
			return -1;

#ifdef DEBUG
		cprintf("cow: 0x%x marked as COW\n", phy);
#endif
	}

	return 0;
}

int vm_is_cow(pgdir_t* dir, vmpage_t page)
{
	/* Does the page exist? */
	if(!vm_findpg(page, 0, dir, 0, 0))
		return 0;
	if(vm_findpgflags(page, dir) & VM_TBL_COWR)
		return 1;
	return 0;
}

int vm_uncow(pgdir_t* dir, vmpage_t page)
{
	/* sanity check */
	if(!vm_is_cow(dir, page))
	{
#ifdef DEBUG
		cprintf("cow: page wasn't COW.\n");
#endif
		return -1;
	}

	pypage_t phy = vm_findpg(page, 0, dir, 0, 0);
	if(!phy) return -1;
	vmflags_t flags = vm_findpgflags(page, dir);
	vmflags_t dirflags = vm_findtblflags(page, dir);

	/* How many references does this page have? */
	int left = vm_pgunshare(phy);

	if(left <= 0)
	{
#ifdef DEBUG
		cprintf("cow: there weren't any refs left for 0x%x\n", phy);
#endif
		/* Just mark the page as writable */
		flags &= ~VM_TBL_COWR;
		if(vm_setpgflags(page, dir, flags | VM_TBL_WRIT))
			return -1;
	} else {
		/* Create a new page */
		pypage_t newpg = palloc();
	
		/* Copy the contents */	
		vm_cpy_page(newpg, phy);

		/* Unset the cow flag */
		if(vm_setpgflags(page, dir, flags & (~VM_TBL_COWR)))
                        return -1;

		/* Unmap the old page */
		pypage_t unmap = vm_unmappage(page, dir);
		if(unmap)pfree(unmap);

		/* Map in the new page */
		vm_mappage(newpg, page, dir, dirflags, flags);
#ifdef DEBUG
		cprintf("cow: reference decremented for 0x%x\n", phy);
#endif
	}

	return 0;
}

#endif
