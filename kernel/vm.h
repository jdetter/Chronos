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
 * 0xFEFF9000 kernel stack lower guard page / user kstack upper guard page
 * 0xFEFF8FFF
 * ...
 * User kstack
 * ...
 * 0xFEFF4000
 * 0xFEFF3000 user kstack lower guard page
 * 0xFEFF2FFF
 * ...
 * Swap stack (stacks of other processes get mapped here)
 * ...
 * 0xFEFEE000
 * 0xFEFED000 extra stack lower guard page
 * 0xFEFFCFFF
 * ...
 * kmalloc space
 * ...
 * 0xFDFF8000
 *
 * 0xFDFF7FFF
 * ...
 * Hardware mapping space
 * ...
 * 0xFD000000
 *  Note: all pages in hardware mapping space are marked as *write through*
 *  Note: all pages below here are directly mapped virt = phy, 
 *
 * VV   Top of the page pool  VV
 * 
 * 0xFEFFFFFF
 * ...
 * Page pool (if it exists)
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
 * 0x0000FDFF
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
 * 0x00000958 Current video mode 
 * 0x00000954 Available pages in the memory pool
 * 0x00000950 Pointer to the first page in the memory pool
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

/** Definitions for the above memory map **/

#define VM_MAX		0xFFFFFFFF /* Maximum possible address */

#define KVM_KERN_E      0xFFFFF000 /* Kernel binary ends */
#define KVM_KERN_S      0xFF000000 /* Kernel binary starts*/

#define KVM_KSTACK_G1   0xFEFFF000 /* Kernel stack upper guard page */

#define KVM_KSTACK_E    0xFEFFEFFF /* Top of the kernel stack */
#define KVM_KSTACK_S    0xFEFFA000 /* Bottom of the kernel stack*/

#define KVM_KSTACK_G2   0xFEFF9000 /* Kernel stack lower guard page */

#define UVM_KSTACK_G1	KVM_KSTACK_G2 /* User kstack upper guard page */
#define UVM_KSTACK_E	0xFEFF8FFF /* End (top) of the user process stack */
#define UVM_KSTACK_S	0xFEFF4000 /* Start of the user process stack */
#define UVM_KSTACK_G2	0xFEFF3000

#define SVM_KSTACK_G1	0xFEFF3000 /* swap stack top guard page */
#define SVM_KSTACK_E	0xFEFF2FFF /* swap stack end */
#define SVM_KSTACK_S	0xFEFEE000 /* swap stack start */
#define SVM_KSTACK_G2	0xFEFED000 /* swap stack bottom guard page */

#define KVM_KMALLOC_E   0xFEFFCFFF /* Where kmalloc ends */
#define KVM_KMALLOC_S   0xFDFF8000 /* Where kmalloc starts */

#define KVM_HARDWARE_E  0xFDFF7FFF /* End of hardware mappings */
#define KVM_HARDWARE_S  0xFD000000 /* Start of hardware mappings */

#define KVM_BOOT2_E	0x0000FDFF /* Start of the second boot stage binary */
#define KVM_BOOT2_S	0x00007E00 /* Start of the second boot stage binary */

#define KVM_KPGDIR      0x00001000 /* The kernel page directory */
#define KVM_VMODE    	0x00000958 /* The current video mode */
#define KVM_PAGE_CT    	0x00000954 /* Amount of pages in the page pool */
#define KVM_POOL_PTR    0x00000950 /* Pointer to the first page in mem pool*/

/* Quickly calculate the difference between the normal and swap stacks. */
#define SVM_DISTANCE (UVM_KSTACK_S - SVM_KSTACK_S)

/** User application memory map
 * 
 * 0xFFFFFFFF Top of kernel space
 * ...
 * Kernel binary space (code + global data)
 * ...
 * 0xFEFFA000
 * 0xFEFF9000 Stack guard page top 
 * 0xFEFF8FFF
 * ...
 * User kernel stack (see definition above)
 * ...
 * 0xFEFF4000
 * 0xFEFF3000 Stack guard page bottom
 * 0xFEFF2FFF
 * ...
 * Hardware mappings, kmalloc space, ...
 * ... 
 * 0xFD000000 Bottom of kernel space
 * 0xFCFFFFFF User application user space stack top
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

/** Definitions for the above memory map **/
#define UVM_KVM_E	0xFFFFFFFF /* End of the kernel space*/
#define UVM_KVM_S	0xFD000000 /* Start of the kernel space */

#define UVM_USTACK_TOP	0xFCFFFFFF /* Top of the user's stack */
#define UVM_LOAD	0x00001000 /* Where the user binary gets loaded */

/** Memory mapped file systems
 *  < Will be updated when implemented >
 */

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

#ifndef __VM_ASM_ONLY__

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
 * Save the current page directory and disable interrupts on this cpu.
 */
pgdir* vm_push_pgdir(void);

/**
 * Restore the previous page directory and pop_cli.
 */
void vm_pop_pgdir(pgdir* dir);

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
 * Add pages to the page pool. This will not add pages to the page pool
 * that are going to be direct mapped.
 */
void vm_add_pages(uint start, uint end, pgdir* dir);

/**
 * Setup the page pool. This is done during the second boot stage.
 */
void vm_init_page_pool(void);

/**
 * Setup the kernel portion of the page table.
 */
void setup_kvm(void);

/**
 * Called when the bootstrap has finished setting up memory. The
 * information saved by this function call is read by the kernel
 * when it starts executing.
 */
void save_vm(void);

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
 * Free the user portion of a page directory.
 */
void vm_free_uvm(pgdir* dir);

/**
 * Use the kernel's page table
 */
void switch_kvm(void);

/**
 * Switch to a user's page table and restore the context.
 */
void switch_uvm(struct proc* p);

/**
 * Map another process's kstack into another process's address space.
 */
void vm_set_user_kstack(pgdir* dir, pgdir* kstack);

/**
 * Map another process's stack into a process's stack swap space.
 */
void vm_set_swap_stack(pgdir* dir, pgdir* swap);

/**
 * Clear this directory's stack swap space.
 */
void vm_clear_swap_stack(pgdir* dir);

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

#endif
