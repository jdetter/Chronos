#include "types.h"
#include "x86.h"
#include "asm.h"
#include "vsfs.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "tty.h"
#include "trap.h"
#include "proc.h"
#include "vm.h"
#include "stdarg.h"
#include "stdlib.h"
#include "panic.h"
#include "console.h"

#define KDIRFLAGS (PGDIR_PRESENT | PGDIR_RW)
#define KTBLFLAGS (PGTBL_PRESENT | PGTBL_RW)
#define UDIRFLAGS (PGDIR_PRESENT | PGDIR_RW | PGDIR_USER)
#define UTBLFLAGS (PGTBL_PRESENT | PGTBL_RW | PGTBL_USER)
#define REGION_COUNT 0x1
#define MAPPING_COUNT 0x2
#define KVM_MAGIC 0x55AA55AA

struct vm_free_node{uint next; uint magic;};
struct kvm_region {uint start; uint end;};
struct kvm_region free_regions[] =
{
	//{0x00000500, 0x00007BFF},
	//{0x00007E00, 0x0007FFFF},
	{KVM_MALLOC, KVM_END} /* 500 MB boundary*/
};

struct kvm_mapping {uint virt; uint phy; uint sz;};
struct kvm_mapping hardware_mappings[] = 
{
	{KVM_COLOR_START, (uint)CONSOLE_COLOR_BASE_ORIG, KVM_COLOR_SZ},
	{KVM_MONO_START, (uint)CONSOLE_MONO_BASE_ORIG, KVM_MONO_SZ},
};

void vm_add_pages(uint start, uint end);
void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user);
void __enable_paging__(uint* pgdir);
void __set_stack__(uint addr);

uint k_pages; /* How many pages are left? */
uint k_stack; /* Kernel stack */
pgdir* k_pgdir; /* Kernel page directory */
static struct vm_free_node* head;

uint vm_init(void)
{
	k_pages = 0;
	head = NULL;
	int x;
	for(x = 0;x < REGION_COUNT;x++)
		vm_add_pages(free_regions[x].start, free_regions[x].end);
	
	/* Initilize the kernel page table */
	k_pgdir = (pgdir*)palloc();
	setup_kvm(k_pgdir);
	
	return k_pages;
}

/* Take pages from start to end and add them to the free list. */
void vm_add_pages(uint start, uint end)
{
	/* Make sure the start and end are page aligned. */
	start = PGROUNDDOWN((start + PGSIZE - 1));
	end = PGROUNDDOWN(end);
	/* There must be at least one page. */
	if(end - start < PGSIZE) return;
	int x;
	for(x = start;x != end;x += PGSIZE) pfree(x);
}

/**
 * Here is the memory map for chronos:
 *
 * 0x00100000 -> 0x001FFFFF Kernel binary space
 * 0x00000000 -> 0x000FFFFF User process space (this changes between procs) 
 *
 * See above for paging memory locations.
 *
 * Segment positions are defined in chronos.h (see SEG_KERNEL_* and SEG_USER_*)
 *
 */

#define GDT_SIZE (sizeof(struct vm_segment_descriptor) * 6)
struct vm_segment_descriptor global_descriptor_table[] ={
	MKVMSEG_NULL, 
	MKVMSEG(SEG_KERN, SEG_EXE , SEG_READ,  0x0, KVM_MAX),
	MKVMSEG(SEG_KERN, SEG_DATA, SEG_WRITE, 0x0, KVM_MAX),
	MKVMSEG(SEG_USER, SEG_EXE , SEG_READ,  0x0, KVM_MAX),
	MKVMSEG(SEG_USER, SEG_DATA, SEG_WRITE, 0x0, KVM_MAX),
	MKVMSEG(SEG_KERN, SEG_DATA, SEG_WRITE, 0x0, 0x0)
};

void vm_seg_init(void)
{
	lgdt((uint)global_descriptor_table, GDT_SIZE);
}

uint palloc(void)
{
	if(head == NULL) panic("No more free pages");
	k_pages--;
	uint addr = (uint)head;
	if(head->magic != (uint)KVM_MAGIC)
	{
		panic("KVM is currupt!\n");
	}
	head = (struct vm_free_node*)head->next;
	memset((uchar*)addr, 0, PGSIZE);

	if(debug) cprintf("Page allocated: 0x%x\n", addr);
	return addr;
}

void pfree(uint pg)
{
	if(debug) cprintf("Page freed: 0x%x\n", pg);
	if(pg == 0) panic("Free null page.\n");
	k_pages++;
	struct vm_free_node* new_free = (struct vm_free_node*)pg;
	new_free->next = (uint)head;
	new_free->magic = (uint)KVM_MAGIC;
	head = new_free;
}

void mappages(uint va, uint sz, pgdir* dir, uchar user)
{
	
	/* round va + sz up to a page */
	uint end = PGROUNDDOWN((va + sz + PGSIZE - 1));
	uint start = PGROUNDDOWN(va);
	if(debug)
		cprintf("Creating virtual mapping from 0x%x to 0x%x\n", 
			start, end);
	if(end <= start) return;
	uint x;
	for(x = start;x != end;x += PGSIZE)
	{
		uint pg = findpg(x, 1, dir, user);
		if(debug) cprintf("Virtual page 0x%x mapped to 0x%x\n", x, pg);
	}
}

void dir_mappages(uint start, uint end, pgdir* dir, uchar user)
{
	start = PGROUNDDOWN(start);
	end = PGROUNDDOWN(end);
	uint x;
	for(x = start;x != end;x += PGSIZE)
		mappage(x, x, dir, user);
}

void setup_kvm(pgdir* dir)
{
	uint start = PGROUNDDOWN(KVM_START);
	uint end = (KVM_END + PGSIZE - 1) & ~(PGSIZE - 1);
	dir_mappages(start, end, k_pgdir, 0);

        /* make sure kmalloc has been mapped into memory. */
        mappages(KVM_KMALLOC_S, KVM_KMALLOC_E - KVM_KMALLOC_S,
                        k_pgdir, 0);

        /* make room for the large kernel stack */
        mappages(KVM_KSTACK_S, KVM_KSTACK_E - KVM_KSTACK_S,
                k_pgdir, 0);

	uint x;
        /* Do hardware mappings */
        for(x = 0;x < MAPPING_COUNT;x++)
                mapping(hardware_mappings[x].phy,
                                hardware_mappings[x].virt,
                                hardware_mappings[x].sz,
                                k_pgdir, 0);
}

void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user)
{
	phy = PGROUNDDOWN(phy);
	virt = PGROUNDDOWN(virt);
	uint end_phy = PGROUNDDOWN(phy + sz + PGSIZE - 1);
	uint end_virt = PGROUNDDOWN(virt + sz + PGSIZE - 1);
	uint pages = end_phy - phy;
	if(end_virt - virt > end_phy - phy)
		pages = end_virt - virt;
	pages /= PGSIZE;

	uint x;
	for(x = 0;x < pages;x++)
		mappage(phy + x, virt + x, dir, user);
}

void mappage(uint phy, uint virt, pgdir* dir, uchar user)
{
	uint dir_flags = KDIRFLAGS;
	uint tbl_flags = KTBLFLAGS;

	if(user)
	{
		dir_flags = UDIRFLAGS;
		tbl_flags = UTBLFLAGS;
	}

	phy = PGROUNDDOWN(phy);
	virt = PGROUNDDOWN(virt);
	uint dir_index = PGDIRINDEX(virt);
	/* Do we need to allocate a new page table? */
	if(!dir[dir_index]) dir[dir_index] = palloc() | dir_flags;

	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	uint tbl_index = PGTBLINDEX(virt);
	if(!tbl[tbl_index])
	{
		tbl[tbl_index] = phy | tbl_flags;
	} else {
		panic("remap");
	}
}

uint findpg(uint virt, int create, pgdir* dir, uchar user)
{
	uint dir_flags = KDIRFLAGS;
	uint tbl_flags = KTBLFLAGS;

	if(user)
	{
		dir_flags = UDIRFLAGS;
		tbl_flags = UTBLFLAGS;
	}

	virt = PGROUNDDOWN(virt);
	uint dir_index = PGDIRINDEX(virt);
	uint tbl_index = PGTBLINDEX(virt);

	if(create && !dir[dir_index]) 
		dir[dir_index] = palloc() | dir_flags;
	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	uint page;
	if(!(page = tbl[tbl_index]))
		page = tbl[tbl_index] = palloc() | tbl_flags;

	return page;
}

void vm_copy_kvm(pgdir* dir)
{
	uint x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
	{
		if(k_pgdir[x])
		{
			dir[x] = palloc() |  KDIRFLAGS;
			uint src_page = PGROUNDDOWN(k_pgdir[x]);
			uint dst_page = PGROUNDDOWN(dir[x]);
			memmove((uchar*)dst_page, (uchar*)src_page, PGSIZE);
		}
	}
}

void vm_copy_vm(pgdir* dst, pgdir* src)
{
        uint x;
        for(x = 0;x < (PGSIZE / sizeof(uint));x++)
        {
                if(src[x])
                {
                        dst[x] = palloc() |  KDIRFLAGS;
                        uint src_page = PGROUNDDOWN(src[x]);
                        uint dst_page = PGROUNDDOWN(dst[x]);
                        memmove((uchar*)dst_page, (uchar*)src_page, PGSIZE);
                }
        }
}

void freepgdir(pgdir* dir)
{
	int dir_index;
	for(dir_index = 0;dir_index < 1024;dir_index++)
	{
		if(dir[dir_index])
		{
			uint* tbl = (uint*)PGROUNDDOWN(dir[dir_index]);
			int tbl_index;
			for(tbl_index = 0;tbl_index < 1024;tbl_index++)
			{
				if(tbl[tbl_index])
				{
					pfree(tbl[tbl_index]);
				}
			}

			pfree(dir[dir_index]);
		}
	}
}

void switch_kvm(void)
{
	//__set_stack__(k_stack);
	__enable_paging__(k_pgdir);
}

void switch_uvm(struct proc* p)
{
	__enable_paging__(p->pgdir);
}

void switch_context(struct proc* p)
{
	/* Set the task segment to point to the process's stacks. */
	uint base = (uint)p->tss;
	uint limit = sizeof(struct task_segment);
	uint access = SEG_DEFAULT_ACCESS | SEG_WRITE;
	uint flag = SEG_SZ;

	global_descriptor_table[SEG_TSS].limit_1 = (uint_16) limit;
	global_descriptor_table[SEG_TSS].base_1 = (uint_16) base;
	global_descriptor_table[SEG_TSS].base_2 = (uint_8)(base>>16);
	global_descriptor_table[SEG_TSS].type = access;
	global_descriptor_table[SEG_TSS].flags_limit_2 = 
		(uint_8)(limit >> 16) | flag;
	global_descriptor_table[SEG_TSS].base_3 = (base >> 24);

	struct task_segment* ts = (struct task_segment*)p->tss;
	ts->SS0 = SEG_KERNEL_DATA << 3;
	ts->ESP0 = base + PGSIZE;

	ltr(SEG_TSS << 3);
	__enable_paging__(p->pgdir);

	if(p->state == PROC_READY)
	{
		/* Switch to the user's stack */
		__set_stack__((uint)(p->tf->esp));
	}

	/* Switch to the user's k stack */
	__set_stack__((uint)(p->tf));
	/* Do the trap return */
	asm volatile("jmp trap_return");
}

void free_list_check(void)
{
	if(debug) cprintf("Checking free list...\t\t\t\t\t\t\t");
	struct vm_free_node* curr = head;
	while(curr)
	{
		if(curr->magic != KVM_MAGIC)
		{
			if(debug) cprintf("[FAIL]\n");
			if(debug) cprintf("[WARNING] KVM "
				"has become unstable.\n");
			return;
		}
		curr = (struct vm_free_node*)curr->next;
	}

	if(debug) cprintf("[ OK ]\n");
}
