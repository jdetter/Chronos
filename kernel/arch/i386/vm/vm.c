/**
 * Author: John Detter <john@detter.com>
 *
 * Virtual memory functions for 80*86
 */

#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "x86.h"
#include "stdlock.h"
#include "trap.h"
#include "devman.h"
#include "proc.h"
#include "vm.h"
#include "k/vm.h"
#include "stdarg.h"
#include "panic.h"
#include "context.h"

/* We need some graphics config for bootup */
#include "k/drivers/console.h"

context_t k_context; /* The kernel context */
pstack_t k_stack; /* Kernel stack */
pgdir_t* k_pgdir; /* Kernel page directory */
int video_mode; /* The video mode dectected on boot */
extern slock_t global_mem_lock;

int vm_init(void)
{
	slock_init(&global_mem_lock);
#ifdef __ALLOW_VM_SHARE__
	vm_share_init(); /* Setup shared memory */
#endif

	/** 
	 * The boot loader has handled all of the messy work for us.
	 * All we need to do is pick up the free map head and kernel
	 * page directory.
	 */
	
	/* The boot strap directly mapped in the null guard page */
	vm_unmappage(0x0, k_pgdir);

	vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT;

	/* Map pages in for our kernel stack */
	vm_mappages(KVM_KSTACK_S, KVM_KSTACK_E - KVM_KSTACK_S, k_pgdir, 
		dir_flags, tbl_flags);

	/* Add bootstrap code to the memory pool (includes stack) */
	int boot2_s = PGROUNDDOWN(BOOT2_S) + PGSIZE;
	int boot2_e = PGROUNDUP(BOOT2_E);

	int x;
	for(x = boot2_s;x < boot2_e;x += PGSIZE)
		pfree(x);	

	/* Clear the TLB */
	vm_enable_paging(k_pgdir);

	return 0;
}

#define GDT_SIZE (sizeof(struct vm_segment_descriptor) * SEG_COUNT)
struct vm_segment_descriptor global_descriptor_table[] = 
{
	MKVMSEG_NULL, 
	MKVMSEG(SEG_KERN, SEG_EXE, SEG_READ,  0x0, VM_MAX),
	MKVMSEG(SEG_KERN, SEG_DATA, SEG_WRITE, 0x0, VM_MAX),
	MKVMSEG(SEG_USER, SEG_EXE, SEG_READ,  0x0, VM_MAX),
	MKVMSEG(SEG_USER, SEG_DATA, SEG_WRITE, 0x0, VM_MAX),
	MKVMSEG_NULL /* Will become TSS*/
};

int vm_seg_init(void)
{
	lgdt((uint)global_descriptor_table, GDT_SIZE);
	return 0;
}
