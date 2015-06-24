#ifndef _UVM_H_
#define _UVM_H_

/*****************************************
 *********** CHRONOS MEMORY MAP **********
 *****************************************/

/** Kernel virtual memory map
 *
 * Build last modified: alpha-0.2
 * Maintainer: John Detter <jdetter@wisc.edu>
 * 
 * 
 * 0xFFFFF000 Virtual memory space top (NO PAE) 
 * ...
 * Kernel binary + global data
 * ...
 * 0xFF000000
 * 0xFEFFF000 kernel stack upper guard page 
 * 0xFEFFEFFF
 * ...
 * Kernel stack
 * ...
 * 0xFEFFA000
 * 0xFEFF9000 kernel stack lower guard page
 * 0xFEFF8FFF 
 * ...
 * kmalloc space
 * ...
 * 0xFDFF8000
 *
 * VV   Top of hardware mapping space  VV
 *
 * ...
 * 0xFDFF7FFF
 * ...
 * 
 * 0xFDFFF000 Monochrome video memory --> 0xB0000
 * 0xFE000000 Color video memory --> 0xB8000
 *
 * ^^ Bottom of hardware mapping space ^^ (for pci, DMA, ect.)
 *  Note: all pages above will be marked as **write through** 
 * 
 *  Note: all pages below here are directly mapped virt = phy, 
 *          all pages above are virtual
 * 
 * VV   Top of the page pool  VV
 * 
 * 0xFDFFFFFF
 * ...
 * Page pool (if it exists)
 * ...
 * 0x01000000 
 *
 * 0x00FFFFFF
 * ...
 * ISA Memory Hole (may or may not be present on modern i386)
 * ...
 * 0x00F00000 
 * 
 * 0x00EFFFFF
 * ... 
 * Page pool
 * ...
 * 0x00100000 
 *
 * 0x000FFFFF
 * ...
 * video memory / BIOS ROM area (Hole)
 * ...
 * 0x000A0000 
 *
 * 0x0009FFFF
 * ...
 * Page pool
 * ...
 * 0x0000FC00
 * 
 * 0x0000FBFF
 * ...
 * Boot stage 2 %%
 * ...
 * 0x00007E00
 * 
 * 0x00007DFF
 * ...
 * Boot stage 1 boot strap %%
 * ...
 * 0x00007C00
 *
 * 0x00007BFF
 * ...
 * Page pool
 * ...
 * 0x00002000 
 * ^^  Bottom of the page pool ^^
 *
 * 0x00001000 Kernel page directory
 * 0x00000950 Pointer to the first page in the memory pool. 
 * 0x00000500 Memory map (boot stage 1)
 * ...
 * All memory here left untouched (real mode IDT, bios data)
 * ...
 * 0x00000000 
 *
 * Notes:
 *   %%: will get reclaimed as part of the memory pool after boot.
 *
 */

/** User application memory map
 * 
 * 0xFFFFFFFF Top of kernel space
 * ...
 * Hardware mappings, kernel code/data, kmalloc space, ...
 * ... 
 * 0xFE000000 Bottom of kernel space
 * 0xFDFFF000 User kernel stack upper guard page
 * 0xFDFFEFFF 
 * ...
 * User kernel stack
 * ...
 * 0xFDFFA000 
 * 0xFDFF9000 User kernel stack lower guard page
 * 0xFDFF8000 User application user space stack top
 *     |
 *     | Stack grows down when the user needs more stack space
 *     |
 *     V
 *     ^
 *     |
 *     | Heap grows up when the user needs more heap space
 *     |
 * ?????????? Start of heap (page aligned after user binary)
 * ...
 * ...
 * 0x00001000 Start of user binary
 * 0x00000000 NULL guard page (Security)
 */

/** Memory mapped file systems
 *  < Will be updated when implemented >
 */

#define PGROUNDDOWN(pg)	((pg) & ~(PGSIZE - 1))	
#define PGROUNDUP(pg)	((pg + PGSIZE - 1) & ~(PGSIZE - 1))	

#define PGDIRINDEX(pg) ((PGROUNDDOWN(pg) >> 22) & 0x3FF)
#define PGTBLINDEX(pg) ((PGROUNDDOWN(pg) >> 12) & 0x3FF)

/**
 * MEMORY MAPPINGS
 *
 * These are the memory mappings that are used by chronos.
 */
#define KVM_START 	0x00100000 /* Where the kernel is loaded */
#define KVM_MALLOC	0x001F0000 /* Where kernel malloc starts*/
#define KVM_END		0x00200000 /* Where the address space ends */
#define KVM_MALLOC_END	KVM_END
#define KVM_MAX		0xFFFFFFFF /* Maximum address */

/* Keep an empty page at the start of the stack */
#define KVM_KSTACK_G1   0x00200000 /* First guard page */
#define KVM_KSTACK_S	0x00201000
#define KVM_KSTACK_E	0x00209000
#define KVM_KSTACK_G2	0x0020A000 /* Second guard page */
/* Keep an empty page at the end of the stack */

#define KVM_COLOR_START 0x0020A000
#define KVM_COLOR_SZ    4000 /* Size of color memory */
#define KVM_MONO_START  0x0020E000      
#define KVM_MONO_SZ     4000 /* Size of mono chrome memory */

/* When a new process is created, what should get transfered? */
#define KVM_CPY_START 	KVM_START
#define KVM_CPY_END 	(KVM_MONO_START + KVM_MONO_SZ)

#define MKVMSEG_NULL {0, 0, 0, 0, 0, 0}
#define MKVMSEG(priv, exe_data, read_write, base, limit) \
        {((limit >> 12) & 0xFFFF),                         \
        (base & 0xFFFF),                                  \
        ((base >> 16) & 0xFF),                            \
        ((SEG_DEFAULT_ACCESS | exe_data | read_write | priv) & 0xFF), \
        (((limit >> 28) | SEG_DEFAULT_FLAGS) & 0xFF),     \
        ((base >> 24) & 0xFF)}

#define TSS_BUSY		0x02
#define TSS_PRESENT		0x80
#define TSS_GRANULARITY		0x80
#define TSS_AVAILABILITY	0x10
#define TSS_DEFAULT_FLAGS	0x09

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
void setup_kvm(void);

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
void mappage(uint phy, uint virt, pgdir* dir, uchar user, uint flags);

/**
 * Unmap the page from the page directory. If there isn't a page there then
 * nothing is done. Returns the page that was unmapped.
 */
uint unmappage(uint virt, pgdir* dir);

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
 * Copy the kernel portion of the page table
 */
void vm_copy_kvm(pgdir* dir);

/**
 * Make an exact copy of the user virtual memory space.
 */
void vm_copy_uvm(pgdir* dst, pgdir* src);

/**
 * Use the kernel's page table
 */
void switch_kvm(void);

/**
 * Switch to a user's page table and restore the context.
 */
void switch_uvm(struct proc* p);

/**
 * Free the user portion of a page directory.
 */
void vm_free_uvm(pgdir* dir);

/**
 * Switch to the user's context.
 */
void switch_context(struct proc* p);

/** Memory debugging functions */
void free_list_check(void); /* Verfy the free list */
void free_list_dump(void); /* Print the free list */

/**
 * Compare 2 page tables.
 */
void pgdir_cmp(pgdir* src, pgdir* dst);

/**
 * Compare 2 pages.
 */
void pg_cmp(uchar* pg1, uchar* pg2);

#endif
