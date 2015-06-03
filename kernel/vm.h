#ifndef _UVM_H_
#define _UVM_H_

#define PGROUNDDOWN(pg)	((pg) & ~(PGSIZE - 1))	
#define PGDIRINDEX(pg) ((PGROUNDDOWN(pg) >> 22) & 0x3FF)
#define PGTBLINDEX(pg) ((PGROUNDDOWN(pg) >> 12) & 0x3FF)

/**
 * MEMORY MAPPINGS
 *
 * These are the memory mappings that are used by chronos.
 */
#define KVM_START 	0x00100000 /* Where the kernel is loaded */
#define KVM_END		0x1F400000 /* Where the address space ends */
#define KVM_MALLOC	0x00200000 /* Where the kvm allocator starts */
#define KVM_MAX		0xFFFFFFFF /* Maximum address */

#define KVM_KMALLOC_S	0x1F500000 /* Start of kmalloc */
#define KVM_KMALLOC_E	0x1F510000 /* end of kmalloc */

#define KVM_COLOR_START	0x20000000
#define KVM_COLOR_SZ	4000 /* Size of color memory */
#define KVM_MONO_START	0x20001000	
#define KVM_MONO_SZ	4000 /* Size of mono chrome memory */	

#define MKVMSEG_NULL {0, 0, 0, 0, 0, 0}
#define MKVMSEG(priv, exe_data, read_write, base, limit) \
        {((limit >> 12) & 0xFFFF),                         \
        (base & 0xFFFF),                                  \
        ((base >> 16) & 0xFF),                            \
        ((SEG_DEFAULT_ACCESS | exe_data | read_write | priv) & 0xFF), \
        (((limit >> 28) | SEG_DEFAULT_FLAGS) & 0xFF),     \
        ((base >> 24) & 0xFF)}

struct vm_segment_descriptor
{
	uint_16 limit_1; /* 0..15 of limit */
	uint_16 base_1;  /* 0..15 of base */
	uint_8  base_2;  /* 16..23 of base */
	uint_8  type;    /* descriptor type */
	uint_8  flags_limit_2; /* flags + limit 16..19 */
	uint_8  base_3; /* 24..31 of base */
};

/**
 * Initilize a free list of pages from address start to address end.
 */
uint vm_init(void);

/**
 * Initilize kernel segments
 */
void vm_seg_init(void);

/**    
 * Allocate a page. Return NULL if there are no more free pages. The address
 * returned should be page aligned.
 * Security: Remember to zero the page before returning the address.
 */
uint palloc(void);

/**
 * Free the page, pg. Add it to anywhere in the free list (no coalescing).
 */
void pfree(uint pg);

/**
 * Setup the kernel portion of the page table.
 */
void setup_kvm(pgdir* dir);

/**
 * Maps pages from va to sz. If certain pages are already mapped, they will be
 * ignored. 
 */
void mappages(uint va, uint sz, pgdir* dir, uchar user);

/**
 * Directly map the pages into the page table.
 */
void dir_mappages(uint start, uint end, pgdir* dir, uchar user);

/**
 * Map the page at virtual address virt, to the physical address phy into the
 * page directory dir. This will create entries and tables where needed.
 */
void mappage(uint phy, uint virt, pgdir* dir, uchar user);

/**
 * Find a page in a page directory. If the page is not mapped, and create is 1,
 * add a new page to the page directory and return the new address. If create
 * is 0, and the page is not found, return 0. Otherwise, return the address
 * of the mapped page.
 */
uint findpg(uint virt, int create, pgdir* dir, uchar user);

/**
 * Free all pages in the page table and free the directory itself.
 */
void freepgdir(pgdir* dir);

/**
 * Use the kernel's page table
 */
void switch_kvm(void);

/**
 * Switch to a user's page table and restore the context.
 */
void switch_uvm(struct proc* p);

#endif
