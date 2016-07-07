#ifndef _VM_ARCH_H_
#define _VM_ARCH_H_

/*****************************************
 *********** CHRONOS MEMORY MAP **********
 *****************************************/

/** Kernel virtual memory map
 *
 * Build last modified: alpha-0.2
 * Maintainer: John Detter <jdetter@chronos.systems>
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
 * 0x000009F0 Current video mode 
 * 0x000009D0 Available pages in the memory pool
 * 0x000009C0 Pointer to the first page in the memory pool
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

#define VM_LAYOUT_PROVIDED
#define VM_MAX			0xFFFFFFFF /* Maximum possible address */

#define KVM_DISK_E      0xFFFFF000 /* End of disk caching space */
#define KVM_DISK_S		0xFFA00000 /* Start of the disk caching space */
#define KVM_KERN_E      0xFF9FFFFF /* Kernel binary ends */
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

#define KVM_KMALLOC_E   0xFE000000 /* Where kmalloc ends */
#define KVM_KMALLOC_S   0xFDFF8000 /* Where kmalloc starts */

#define KVM_HARDWARE_E  0xFDFF7FFF /* End of hardware mappings */
#define KVM_HARDWARE_S  0xFD000000 /* Start of hardware mappings */

/* Booting addresses */

/* The end of the second stage boot loader (exclusive) */
#define BOOT2_E			0x0001B000

#define BOOT2_BSS_E		0x0001AFFF /* End of BSS section */
#define BOOT2_BSS_S		0x0001A000 /* Start of BSS section */
#define BOOT2_DATA_E	0x00019FFF /* End of data section */
#define BOOT2_DATA_S	0x00019000 /* Start of data section */
#define BOOT2_RODATA_E	0x00018FFF /* End of read only data section */
#define BOOT2_RODATA_S	0x00018000 /* Start of the read only data section */
#define BOOT2_TEXT_E	0x00017FFF /* End of the code section */
#define BOOT2_TEXT_S	0x00008000 /* Start of the code section */
#define BOOT2_STACK_TOP	0x00008000 /* Where the second boot stage stack starts */
#define BOOT2_STACK_BOT	0x00007000 /* The page boundary for the boot stage 2 stack */
#define BOOT2_E820_E	0x00006FFF /* The end of the e820 table */
#define BOOT2_E820_S	0x00006000 /* The start of the e820 table */
#define BOOT2_KARGS		0x00005000 /* Page of kernel arguments */

#define BOOT2_S			0x00005000 /* The first page of the stage 2 loader */

#define KVM_KPGDIR      0x00001000 /* The kernel page directory */
#define KVM_VMODE    	0x00005000 /* The current video mode */
#define KVM_PAGE_CT    	0x00005004 /* Amount of pages in the page pool */
#define KVM_POOL_PTR    0x00005008 /* Pointer to the first page in mem pool*/

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
 * 0xFCFFFFFF User application user space top
 * ...
 * ?????????? Environment variable space (dependant on size)
 * ?????????? User stack start
 *     |
 *     | Stack grows down when the user needs more stack space
 *     |
 *     V
 * ?????????? mmap area start
 * ...
 * ?????????? mmap area end
 *     |
 *     | mmap area grows towards heap
 *     |
 *     V
 *
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
#define UVM_TOP		0xFCFFFFFF /* Top of the user address space */
/* User stack start is now unknown due to environment variables */
//#define UVM_USTACK_TOP	0xFCFFFFFF /* Top of the user's stack */
#define UVM_LOAD	0x00001000 /* Where the user binary gets loaded */

#define UVM_MIN_STACK	0x00A00000 /* 10MB minimum stack size */


/* Some page table definitions */
#define PGSIZE 4096
#define PGSHIFT 12
#define PGROUNDDOWN(pg) ((pg) & ~(PGSIZE - 1))  
#define PGROUNDUP(pg)   ((pg + PGSIZE - 1) & ~(PGSIZE - 1))

#define PGDIRINDEX(pg) ((PGROUNDDOWN(pg) >> 22) & 0x3FF)
#define PGTBLINDEX(pg) ((PGROUNDDOWN(pg) >> 12) & 0x3FF)

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


/** Allow vm sharing */
#ifndef __BOOT_STRAP__
// #define __ALLOW_VM_SHARE__
#endif

#ifndef __ASM_ONLY__

#include <stdint.h>

typedef unsigned int vmflags_t;
typedef uintptr_t pgdir_t;
typedef uintptr_t pgtbl_t;
typedef uintptr_t vmpage_t;
typedef uintptr_t pypage_t;
typedef uintptr_t pstack_t;

struct vm_segment_descriptor
{
        uint16_t limit_1; /* 0..15 of limit */
        uint16_t base_1;  /* 0..15 of base */
        uint8_t  base_2;  /* 16..23 of base */
        uint8_t  type;    /* descriptor type */
        uint8_t  flags_limit_2; /* flags + limit 16..19 */
        uint8_t  base_3; /* 24..31 of base */
};

extern pgdir_t* k_pgdir;
extern int video_mode;

/**
 * Save the current page directory and disable interrupts on this cpu. This
 * is to prevent a situation where the current thread is interrupted while
 * we are changing the page directory.
 */
extern pgdir_t* vm_push_pgdir(void);

/**
 * Restore the previous page directory and pop_cli. This does the opposite
 * action of vm_push_pgdir. Even if vm_push_pgdir didn't change the current
 * page directory, vm_pop_pgdir should still be called. Not calling
 * vm_pop_pgdir after a vm_push_pgdir could cause the cpu to lose
 * interrupts.
 */
extern void vm_pop_pgdir(pgdir_t* dir);

/**
 * Initilize segmentation if it is used. Returns 0 on success, non zero
 * otherwise.
 */
extern int vm_seg_init(void);

/**
 * Initilize the page allocator.
 */
extern void vm_alloc_init(void);

/**
 * Save the state of the page allocator. This is only done during
 * the second boot stage.
 */
extern void vm_alloc_save_state(void);

/**
 * Restore the state of the page allocator. This is only done during
 * kernel boot.
 */
extern void vm_alloc_restore_state(void);

/**
 * Setup the page pool. This is done during the boot strap.
 */
extern void vm_init_page_pool(void);

/**
 * Add pages to the page pool. This will not add pages to the page pool
 * that are going to be direct mapped.
 */
extern void vm_add_pages(vmpage_t start, vmpage_t end, pgdir_t* dir);

/**
 * Enable paging and set the current page table directory.
 */
extern void vm_enable_paging(pgdir_t* pgdir);

/**
 * Diable paging and unset the current page table directory.
 */
extern void vm_disable_paging(void);


/**
 * Enable kernel readonly protection. Exceptions will now be thrown
 * if the kernel attempts to write to a readonly page that is owned
 * by the kernel.
 */
extern void vm_enforce_kernel_readonly(void);

/**
 * Check to see if paging is enabled. Returns zero if paging is disabled, 
 * nonzero otherwise.
 */
extern int vm_check_paging(void);

/**
 * Set the stack of the runing program and jump to the start of the
 * given program. This function does not return.
 */
extern void vm_set_stack(uintptr_t stack, void* callback) __attribute__ ((noreturn));

/**
 * If there was a page fault, returns the address access that caused
 * the fault to occur. This is just a mapping to x86_get_cr2.
 */
#define vm_get_page_fault_address x86_get_cr2

#endif /* !ASM_ONLY */

/** Prevent the generic vm.h from checking for this file */
#define __NO_VM_ARCH__

#ifndef _VM_H_
#include "k/vm.h"
#endif

#endif
