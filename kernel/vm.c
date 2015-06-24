#include "types.h"
#include "x86.h"
#include "asm.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "trap.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
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

struct kvm_mapping {uint virt; uint phy; uint sz;};
struct kvm_mapping hardware_mappings[] = 
{
	{KVM_COLOR_START, (uint)CONSOLE_COLOR_BASE_ORIG, KVM_COLOR_SZ},
	{KVM_MONO_START, (uint)CONSOLE_MONO_BASE_ORIG, KVM_MONO_SZ},
};

void vm_add_page(uint pg);
void vm_add_pages(uint start, uint end);
void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user, uint flags);
void vm_safe_mapping(uint start, uint end, pgdir* dir);
pgdir* __get_cr3__(void);
uchar __check_paging__(void);
void __enable_paging__(uint* pgdir);
void __set_stack__(uint addr);
void __drop_priv__(uint* k_context, uint new_esp);
void __context_restore__(uint* current, uint old);

uint k_context; /* The kernel context */
uint k_pages; /* How many pages are left? */
uint k_stack; /* Kernel stack */
pgdir* k_pgdir; /* Kernel page directory */
static struct vm_free_node* head;

#define E820_UNUSED 	0x01
#define E820_RESERVED 	0x02
#define E820_ACPI_REC	0x03
#define E820_ACPI_NVS	0x04
#define E820_CURRUPT	0x05

#define E820_MAP_START	0x500

struct vm_no_malloc
{
	uint start_addr;
	uint end_addr;
};

/**
 * Page that should NOT be added to the page pool.
 */
#define VM_NO_MALLOC_COUNT 0x04
struct vm_no_malloc vm_no_malloc_table[] = {
	{KVM_START, KVM_END},
	{KVM_COLOR_START, KVM_COLOR_START + KVM_COLOR_SZ},
	{KVM_MONO_START, KVM_MONO_SZ},
	/* Kernel stack needs to be protected with a page above and below. */
	{KVM_KSTACK_S - PGSIZE, KVM_KSTACK_E + PGSIZE}
};

/**
 * Memory map entries from BIOS call int 15h, eax e820h
 */
struct e820_entry
{
	uint_32 addr_low;
	uint_32 addr_high;
	uint_32 length_low;
	uint_32 length_high;
	uint_32 type;
	uint_32 acpi_attr;
};

uint vm_init(void)
{
	k_pages = 0;
	head = NULL;
	/* Get the information from the memory map. */
	int x = E820_MAP_START;
	for(;;x += sizeof(struct e820_entry))
	{
		struct e820_entry* e = (struct e820_entry*)x;
		/* Is this the end of the list? */
		if(e->type == 0x00) break;
		/* look at the type first, make sure it's ok to use. */
		if(e->type != E820_UNUSED) continue;

		/* The entry is free to use! */
		uint addr_start = e->addr_low;
		uint addr_end =	addr_start + e->length_low;


		if(addr_start == 0x0) addr_start += PGSIZE;
		vm_add_pages(addr_start, addr_end);
	}	

	/* Initilize the kernel page table */
	k_pgdir = (pgdir*)palloc();
	setup_kvm();
	
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
	for(x = start;x != end;x += PGSIZE) vm_add_page(x);
}

void vm_add_page(uint pg)
{
        int x;
        for(x = 0;x < VM_NO_MALLOC_COUNT;x++)
        {
                struct vm_no_malloc* m = vm_no_malloc_table + x;
                if(pg >= m->start_addr && pg < m->end_addr) 
		{
			cprintf("Page avoided: 0x%x\n", pg);
			return;
		}
        }

        pfree(pg);
}

#define GDT_SIZE (sizeof(struct vm_segment_descriptor) * SEG_COUNT)
struct vm_segment_descriptor global_descriptor_table[] ={
	MKVMSEG_NULL, 
	MKVMSEG(SEG_KERN, SEG_EXE, SEG_READ,  0x0, KVM_MAX),
	MKVMSEG(SEG_KERN, SEG_DATA, SEG_WRITE, 0x0, KVM_MAX),
	MKVMSEG(SEG_USER, SEG_EXE, SEG_READ,  0x0, KVM_MAX),
	MKVMSEG(SEG_USER, SEG_DATA, SEG_WRITE, 0x0, KVM_MAX),
	MKVMSEG_NULL /* Will become TSS*/
};

void vm_seg_init(void)
{
	lgdt((uint)global_descriptor_table, GDT_SIZE);
}

uint palloc(void)
{
	if(head == NULL) panic("No more free pages");
	/* Is paging enabled? */
	pgdir* old = NULL;
	if(__check_paging__())
	{
		old = __get_cr3__();
		if(old == k_pgdir)
			old = NULL;
		else	
		{
			push_cli();
			__enable_paging__(k_pgdir);
		}
	}

	k_pages--;
	uint addr = (uint)head;
	if(head->magic != (uint)KVM_MAGIC)
	{
		panic("KVM is currupt!\n");
	}
	head = (struct vm_free_node*)head->next;
	memset((uchar*)addr, 0, PGSIZE);

	if(debug) cprintf("Page allocated: 0x%x\n", addr);
	if(old)
	{
		__enable_paging__(old);
		pop_cli();
	}

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
	for(x = start;x < end;x += PGSIZE)
		mappage(x, x, dir, user, 0);
}

void setup_kvm(void)
{
	uint start = PGROUNDDOWN(KVM_START);
	uint end = (KVM_END + PGSIZE - 1) & ~(PGSIZE - 1);
	dir_mappages(start, end, k_pgdir, 0);

	/* make room for the large kernel stack */
	mappages(KVM_KSTACK_S, KVM_KSTACK_E - KVM_KSTACK_S,
			k_pgdir, 0);

	uint x;
	/* Do hardware mappings */
	for(x = 0;x < MAPPING_COUNT;x++)
		mapping(hardware_mappings[x].phy,
				hardware_mappings[x].virt,
				hardware_mappings[x].sz,
				k_pgdir, 0, PGDIR_WTHROUGH);

	x = E820_MAP_START;
	for(;;x += sizeof(struct e820_entry))
	{
		struct e820_entry* e = (struct e820_entry*)x;
		/* Is this the end of the list? */
		if(e->type == 0x00) break;
		/* look at the type first, make sure it's ok to use. */
		if(e->type != E820_UNUSED) continue;

		/* directly map this entry */
		uint addr_start = e->addr_low;
		uint addr_end = addr_start + e->length_low;

		if(addr_start == 0x0) addr_start += PGSIZE;
		vm_safe_mapping(addr_start, addr_end, k_pgdir);
	}
}

void vm_safe_mapping(uint start, uint end, pgdir* dir)
{
	start = PGROUNDDOWN(start);
	end = PGROUNDUP(end);
	uint page;
	for(page = start;page < end;page += PGSIZE)
	{
		/* Security: do not directly map guard pages */
		if(page == KVM_KSTACK_G1 || page == KVM_KSTACK_G2)
			continue;
		if(!findpg(page, 0, k_pgdir, 0))
			mappage(page, page, k_pgdir, 0, 0);
	}
}

void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user, uint flags)
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
		mappage(phy + x, virt + x, dir, user, flags);
}

void mappage(uint phy, uint virt, pgdir* dir, uchar user, uint flags)
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
		tbl[tbl_index] = phy | tbl_flags | flags;
	} else {
		panic("kvm remap: 0x%x\n", virt);
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

	if(!dir[dir_index]) 
	{	
		if(create) dir[dir_index] = palloc() | dir_flags;
		else return 0;
	}

	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	uint page;
	if(!(page = tbl[tbl_index]))
	{
		if(create) page = tbl[tbl_index] = palloc() | tbl_flags;
		else return 0;
	}

	return PGROUNDDOWN(page);
}

uint unmappage(uint virt, pgdir* dir)
{
	virt = PGROUNDDOWN(virt);
	uint dir_index = PGDIRINDEX(virt);
	uint tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index]) return 0;

	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	if(tbl[tbl_index])
	{
		uint page = PGROUNDDOWN(tbl[tbl_index]);
		tbl[tbl_index] = 0;
		return page;
	}

	return 0;
}

void vm_copy_kvm(pgdir* dir)
{
	uint page;
	for(page = KVM_CPY_START;page < KVM_CPY_END;page += PGSIZE)
	{
		uint dir_index = PGDIRINDEX(page);
        	uint tbl_index = PGTBLINDEX(page);

		if(!k_pgdir[dir_index]) continue;

		pgtbl* table = (uint*)PGROUNDDOWN(k_pgdir[dir_index]);
		uint phy = PGROUNDDOWN(table[tbl_index]);
		uint flags = table[tbl_index] - phy;
		if(phy) 
			mappage(phy, page, dir, 0, flags);
	}
}

void vm_copy_uvm(pgdir* dst_dir, pgdir* src_dir)
{
	uint x;
	for(x = 0;x < KVM_START;x += PGSIZE)
	{
		uint src_page = findpg(x, 0, src_dir, 1);
		if(!src_page) continue;

		uint dst_page = findpg(x, 1, dst_dir, 1);
		memmove((uchar*)dst_page,
				(uchar*)src_page,
				PGSIZE);
	}
}

void vm_free_uvm(pgdir* dir)
{
	uint x;
	for(x = 0;x < KVM_START;x += PGSIZE)
	{
		uint page = unmappage(x, dir);
		if(page) pfree(page);
	}
}

void freepgdir(pgdir* dir)
{
	/* Free user pages */
	vm_free_uvm(dir);
	/* Free kernel pages */
	uint x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
	{
		if(dir[x])
		{
			pfree(PGROUNDDOWN(dir[x]));
			dir[x] = 0;
		}
	}

	/* free directory */
	pfree((uint)dir);
}

void switch_kvm(void)
{
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
	uint type = TSS_DEFAULT_FLAGS | TSS_PRESENT;
	uint flag = TSS_AVAILABILITY;

	global_descriptor_table[SEG_TSS].limit_1 = (uint_16) limit;
	global_descriptor_table[SEG_TSS].base_1 = (uint_16) base;
	global_descriptor_table[SEG_TSS].base_2 = (uint_8)(base>>16);
	global_descriptor_table[SEG_TSS].type = type;
	global_descriptor_table[SEG_TSS].flags_limit_2 = 
		(uint_8)(limit >> 16) | flag;
	global_descriptor_table[SEG_TSS].base_3 = (base >> 24);

	struct task_segment* ts = (struct task_segment*)p->tss;
	ts->SS0 = SEG_KERNEL_DATA << 3;
	ts->ESP0 = base + PGSIZE;
	ts->esp = p->tf->esp;
	ts->ss = SEG_USER_DATA << 3;

	__enable_paging__(p->pgdir);
	ltr(SEG_TSS << 3);

	if(p->state == PROC_READY)
	{
		p->state = PROC_RUNNING;
		__drop_priv__(&k_context, p->tf->esp);

		/* When we get back here, the process is done running for now. */
		return;
	}
	if(p->state != PROC_RUNNABLE)
		panic("Tried to schedule non-runnable process.");

	p->state = PROC_RUNNING;

	/* Go from kernel context to process context */
	__context_restore__(&k_context, p->context);
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

void pg_cmp(uchar* pg1, uchar* pg2)
{
	cprintf("INDEX     | PAGE 1 | PAGE 2 \n");
	int x;
	for(x = 0;x < PGSIZE;x++)
		cprintf("0x%x | 0x%x | 0x%x \n", x, pg1[x], pg2[x]);
}

void pgdir_cmp(pgdir* src, pgdir* dst)
{
	cprintf("INDEX      | DIRECTORY 1 | DIRECTORY 2\n");
	uint x;
	uchar dot = 1;
	for(x = 0;x < KVM_START;x += PGSIZE)
	{
		uint src_page = findpg(x, 0, src, 0);
		uint dst_page = findpg(x, 0, dst, 0);

		if(src_page || dst_page)
		{
			dot = 1;
			cprintf("0x%x | ", x);
			cprintf("0x%x | ", src_page);
			cprintf("0x%x\n", dst_page);
		} else {
			if(dot)
			{
				cprintf("0x%x | ", x);
				cprintf(".......... | ");
				cprintf("..........\n");
				dot = 0;
			}
		}
	}
}
