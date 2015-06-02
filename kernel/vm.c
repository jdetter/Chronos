#include "types.h"
#include "x86.h"
#include "asm.h"
#include "vm.h"
#include "stdlib.h"
#include "panic.h"

#define KDIRFLAGS (PGDIR_PRESENT | PGDIR_RW)
#define KTBLFLAGS (PGTBL_PRESENT | PGTBL_RW)
#define UDIRFLAGS (PGDIR_PRESENT | PGDIR_RW | PGDIR_USER)
#define UTBLFLAGS (PGTBL_PRESENT | PGTBL_RW | PGTBL_USER)

void __enable_paging__(uint* pgdir);
pgdir* kernel_pgdir;

struct vm_free_node
{
	uint next;
};

struct vm_free_node* head;

uint vm_init(uint start, uint end)
{
	/* Make sure the start and end are page aligned. */
	start = PGROUNDDOWN(start);
	end = PGROUNDDOWN(end);

	/* There must be at least one page. */
	if(end - start < PGSIZE) return 0;
	
	/* Set the start of the list */
	head = (struct vm_free_node*)start;
	struct vm_free_node* curr = head;
	uint addr = start + PGSIZE; /* The address of the next page */
	for(;addr <= (end - PGSIZE);addr += PGSIZE)
	{
		curr->next = addr;
		curr = (struct vm_free_node*)(((uchar*)curr) + PGSIZE);
	}

	curr->next = 0; /* Set the end of the list */

	/* Return the amount of pages added to the list */
	return (end - start) / PGSIZE;
}

/**
 * Here is the memory map for chronos:
 *
 * 0x00200000 -> 0x00EFFFFF Kernel paging space
 * 0x00100000 -> 0x001FFFFF Kernel binary space
 * 0x00000000 -> 0x000FFFFF User process space (this changes between procs) 
 */


#define GDT_SIZE (sizeof(struct vm_segment_descriptor) * 5)
struct vm_segment_descriptor table[] ={
	MKVMSEG_NULL, 
	MKVMSEG(SEG_KERN, SEG_EXE , SEG_READ,  0x0, KVM_MAX),
	MKVMSEG(SEG_KERN, SEG_DATA, SEG_WRITE, 0x0, KVM_MAX),
	MKVMSEG(SEG_USER, SEG_EXE , SEG_READ,  0x0, KVM_START),
	MKVMSEG(SEG_USER, SEG_DATA, SEG_WRITE, 0x0, KVM_START)
};

void vm_seg_init(void)
{
	lgdt((uint)table, GDT_SIZE);	
}

uint palloc(void)
{
	if(head == NULL) panic("No more free pages");
	uint addr = (uint)head;
	head = (struct vm_free_node*)head->next;
	memset((uchar*)addr, 0, PGSIZE);
	return addr;
}

void pfree(uint pg)
{
	if(pg == 0) panic("Free null page.\n");
	struct vm_free_node* new_free = (struct vm_free_node*)pg;
	new_free->next = (uint)head;
	head = new_free;
}

void mappages(uint va, uint sz, pgdir* dir, uchar user)
{

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
	__enable_paging__(kernel_pgdir);
}

void switch_uvm(pgdir* dir)
{
	__enable_paging__(dir);

}
