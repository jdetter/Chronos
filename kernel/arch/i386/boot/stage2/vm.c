/**
 * Author: John Detter <jdetter@chronos.systems>
 *
 * Virtual memory functions that are used during boot stage 2 and
 * the booting of the kernel.
 */

#include <string.h>

#include "vm.h"
#include "stdlock.h"

#define E820_UNUSED     0x01
#define E820_RESERVED   0x02
#define E820_ACPI_REC   0x03
#define E820_ACPI_NVS   0x04
#define E820_CURRUPT    0x05

/**
 * Memory map entries from BIOS call int 15h, eax e820h
 */
struct e820_entry
{
	uint32_t addr_low;
	uint32_t addr_high;
	uint32_t length_low;
	uint32_t length_high;
	uint32_t type;
	uint32_t acpi_attr;
};

struct mem_region 
{
	vmpage_t start_addr; 
	vmpage_t end_addr;
};

/**
 * This table defines the areas in memory that shouldn't be added
 * to the initial page pool. These areas will be currupted when
 * added to the page pool so anything that the boot loader is going
 * to use should be added to this table. 
 */
#define VM_IGNORE_TABLE_COUNT 3
struct mem_region vm_ignore_table[] = {
	{PGROUNDDOWN(UVM_KVM_S), PGROUNDUP(UVM_KVM_E)}, /* Kernel mapping */
	{PGROUNDDOWN(BOOT2_S), PGROUNDUP(BOOT2_E - 1)}, /* Boot stage 2 code and data */
	{0x00000000, 0x00002000} /* Null page + k_pgdir_t*/
};

pgdir_t* k_pgdir; /* Kernel page directory */
int video_mode; /* The video mode during bootup */

static void vm_stable_page_pool(void)
{
	struct e820_entry* entry = (void*)BOOT2_E820_S;
	entry->addr_low =    0x00000000;
	entry->addr_high =   0x00000000;
	entry->length_low =  0x9fc00;
	entry->length_high = 0x00000000;
	entry->type = 1;
	entry->acpi_attr = 1;
	entry++;
	entry->addr_low =    0x100000;
	entry->addr_high =   0x00000000;
	entry->length_low =  0x10000000;
	//entry->length_low =  0x1fffe000;
	entry->length_high = 0x00000000;
	entry->type = 1;
	entry->acpi_attr = 1;
	entry++;

	/* Make a null entry */
	memset(entry, 0, sizeof(struct e820_entry));
}

static void vm_add_page(vmpage_t pg, pgdir_t* dir)
{
	/* Should we ignore this page? */
	int x;
	for(x = 0;x < VM_IGNORE_TABLE_COUNT;x++)
	{
		struct mem_region* m = vm_ignore_table + x;

		if(pg >= m->start_addr && pg < m->end_addr) 
			return;
	}

	pfree(pg);
	/* If there is a dir, map this page into the dir */
	if(dir) 
	{
		vm_dir_mappages(pg, pg + PGSIZE, dir,
			VM_DIR_WRIT, VM_TBL_WRIT);
	}
}

/* Take pages from start to end and add them to the free list. */
void vm_add_pages(vmpage_t start, vmpage_t end, pgdir_t* dir)
{
	/* Make sure the start and end are page aligned. */
	start = PGROUNDDOWN((start + PGSIZE - 1));
	end = PGROUNDDOWN(end);
	/* There must be at least one page. */
	if(end - start < PGSIZE) return;
	int x;
	for(x = start;x != end;x += PGSIZE) vm_add_page(x, dir);
}

void vm_init_page_pool(void)
{
	/* E820 is broken right now */
	vm_stable_page_pool();

	/* Clear the page allocator */
	k_pgdir = (pgdir_t*)KVM_KPGDIR;

	/* Get the information from the memory map. */
	int x = BOOT2_E820_S;
	for(;;x += sizeof(struct e820_entry))
	{
		struct e820_entry* e = (struct e820_entry*)x;
		/* Is this the end of the list? */
		if(e->type == 0x00) break;
		/* look at the type first, make sure it's ok to use. */
		if(e->type != E820_UNUSED) continue;

		/* The entry is free to use! */
		vmpage_t addr_start = e->addr_low;
		vmpage_t addr_end = addr_start + e->length_low;

		if(addr_start == 0x0) addr_start += 0x1000;
		vm_add_pages(addr_start, addr_end, k_pgdir);
	}
}
