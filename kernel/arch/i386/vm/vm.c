/**
 * Author: John Detter <john@detter.com>
 *
 * Virtual memory functions for 80*86
 */

#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include <kern/stdlib.h>
#include "x86.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "trap.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "stdarg.h"
#include "panic.h"
#include "drivers/console.h"

void __set_stack__(uint addr);
void __drop_priv__(uint* k_context, uint new_esp, uint new_eip);
void __context_restore__(uint* current, uint old);

uint k_context; /* The kernel context */
uint k_stack; /* Kernel stack */
extern pgdir_t* k_pgdir; /* Kernel page directory */
extern slock_t global_mem_lock;

int vm_init(void)
{
	slock_init(&global_mem_lock);
#ifdef _ALLOW_VM_SHARE_
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

	/* Map pages in for kmalloc */
	// vm_mappages(KVM_KMALLOC_S, KVM_KMALLOC_E - KVM_KMALLOC_S, 
	//	k_pgdir, dir_flags, tbl_flags);

	/* Add bootstrap code to the memory pool */
	int boot2_s = PGROUNDDOWN(KVM_BOOT2_S) + PGSIZE;
	int boot2_e = PGROUNDUP(KVM_BOOT2_E);

	int x;
	for(x = boot2_s;x < boot2_e;x += PGSIZE)
		pfree(x);	
	/* Clear the TLB */
	proc_switch_kvm();

	return 0;
}

#define GDT_SIZE (sizeof(struct vm_segment_descriptor) * SEG_COUNT)
struct vm_segment_descriptor global_descriptor_table[] ={
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

void switch_kvm(void)
{
	vm_enable_paging(k_pgdir);
}

void switch_uvm(struct proc* p)
{
	vm_enable_paging(p->pgdir);
}

void switch_context(struct proc* p)
{
	/* Set the task segment to point to the process's stacks. */
	uint base = (uint)p->tss;
	uint limit = sizeof(struct task_segment);
	uint type = TSS_DEFAULT_FLAGS | TSS_PRESENT;
	uint flag = TSS_AVAILABILITY;

	global_descriptor_table[SEG_TSS].limit_1 = (uint_16) limit;
	global_descriptor_table[SEG_TSS].base_1 = (uint_16) base;
	global_descriptor_table[SEG_TSS].base_2 = (uint_8)(base>>16);
	global_descriptor_table[SEG_TSS].type = type;
	global_descriptor_table[SEG_TSS].flags_limit_2 = 
		(uint_8)(limit >> 16) | flag;
	global_descriptor_table[SEG_TSS].base_3 = (base >> 24);

	/* Switch to the user's page directory (stack access) */
	vm_enable_paging(p->pgdir);
	struct task_segment* ts = (struct task_segment*)p->tss;
	ts->SS0 = SEG_KERNEL_DATA << 3;
	ts->ESP0 = (uint)p->k_stack;
	ts->esp = p->tf->esp;
	ts->ss = SEG_USER_DATA << 3;
	ltr(SEG_TSS << 3);

	if(p->state == PROC_READY)
	{
		p->state = PROC_RUNNING;
		__drop_priv__(&k_context, p->tf->esp, p->entry_point);

		/* When we get back here, the process is done running for now. */
		return;
	}
	if(p->state != PROC_RUNNABLE)
		panic("Tried to schedule non-runnable process.");

	p->state = PROC_RUNNING;

	/* Go from kernel context to process context */
	__context_restore__(&k_context, p->context);
}
