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

#define VM_LAYOUT_PROVIDED
#define VM_MAX		0xFFFFFFFF /* Maximum possible address */

#define KVM_DISK_E      0xFFFFF000 /* End of disk caching space */
#define KVM_DISK_S	0xFFA00000 /* Start of the disk caching space */
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

#define KVM_KMALLOC_E   0xFEFFCFFF /* Where kmalloc ends */
#define KVM_KMALLOC_S   0xFDFF8000 /* Where kmalloc starts */

#define KVM_HARDWARE_E  0xFDFF7FFF /* End of hardware mappings */
#define KVM_HARDWARE_S  0xFD000000 /* Start of hardware mappings */

#define KVM_BOOT2_E	0x00019600 /* Start of the second boot stage binary */
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
 * Turn generic flags for a page directory into flags for an i386
 * page directory. Returns the correcponding flags for an i386 page
 * table.
 */
pgflags_t vm_dir_flags(pgflags_t flags);

/**
 * Turn generic flags for a page table into flags for an i386
 * page table. Returns the corresponding flags for an i386 page
 * table.
 */
pgflags_t vm_tbl_flags(pgflags_t flags);

#endif

#endif
