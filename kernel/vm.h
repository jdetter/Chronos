#ifndef _UVM_H_
#define _UVM_H_

/* Include the architecture independant header */
#ifdef ARCH_i386
#include  "arch/i386/vm/vm.h"
#else
#error "Invalid architecture selected."
#endif

/** VERY GENERIC MEMORY MAPS */
#ifndef VM_LAYOUT_PROVIDED

#define VM_LAYOUT_PROVIDED
#define VM_MAX          0xFFFFFFFF /* Maximum possible address */

#define KVM_DISK_E      0xFFFFF000 /* End of disk caching space */
#define KVM_DISK_S      0xFFA00000 /* Start of the disk caching space */
#define KVM_KERN_E      0xFF9FFFFF /* Kernel binary ends */
#define KVM_KERN_S      0xFF000000 /* Kernel binary starts*/

#define KVM_KSTACK_G1   0xFEFFF000 /* Kernel stack upper guard page */

#define KVM_KSTACK_E    0xFEFFEFFF /* Top of the kernel stack */
#define KVM_KSTACK_S    0xFEFFA000 /* Bottom of the kernel stack*/

#define KVM_KSTACK_G2   0xFEFF9000 /* Kernel stack lower guard page */

#define UVM_KSTACK_G1   KVM_KSTACK_G2 /* User kstack upper guard page */
#define UVM_KSTACK_E    0xFEFF8FFF /* End (top) of the user process stack */
#define UVM_KSTACK_S    0xFEFF4000 /* Start of the user process stack */
#define UVM_KSTACK_G2   0xFEFF3000

#define SVM_KSTACK_G1   0xFEFF3000 /* swap stack top guard page */
#define SVM_KSTACK_E    0xFEFF2FFF /* swap stack end */
#define SVM_KSTACK_S    0xFEFEE000 /* swap stack start */
#define SVM_KSTACK_G2   0xFEFED000 /* swap stack bottom guard page */

#define KVM_KMALLOC_E   0xFEFFCFFF /* Where kmalloc ends */
#define KVM_KMALLOC_S   0xFDFF8000 /* Where kmalloc starts */

#define KVM_HARDWARE_E  0xFDFF7FFF /* End of hardware mappings */
#define KVM_HARDWARE_S  0xFD000000 /* Start of hardware mappings */

#define KVM_BOOT2_E     0x00019600 /* Start of the second boot stage binary */
#define KVM_BOOT2_S     0x00007E00 /* Start of the second boot stage binary */

#define KVM_KPGDIR      0x00001000 /* The kernel page directory */
#define KVM_VMODE       0x00000958 /* The current video mode */
#define KVM_PAGE_CT     0x00000954 /* Amount of pages in the page pool */
#define KVM_POOL_PTR    0x00000950 /* Pointer to the first page in mem pool*/

/* Quickly calculate the difference between the normal and swap stacks. */
#define SVM_DISTANCE (UVM_KSTACK_S - SVM_KSTACK_S)

/** Definitions for the above memory map **/
#define UVM_KVM_E       0xFFFFFFFF /* End of the kernel space*/
#define UVM_KVM_S       0xFD000000 /* Start of the kernel space */
#define UVM_TOP         0xFCFFFFFF /* Top of the user address space */
/* User stack start is now unknown due to environment variables */
//#define UVM_USTACK_TOP        0xFCFFFFFF /* Top of the user's stack */
#define UVM_LOAD        0x00001000 /* Where the user binary gets loaded */

#define UVM_MIN_STACK   0x00A00000 /* 10MB minimum stack size */

#endif

/* GENERIC DIRECTORY FLAGS */
#define VM_DIR_GLBL 0x0001 /* Mark the table as a global page */
#define VM_DIR_DRTY 0x0002 /* Mark the table as dirty */
#define VM_DIR_ACSS 0x0004 /* Mark the table as accessed */
#define VM_DIR_CACH 0x0008 /* Mark the table as not cached */
#define VM_DIR_WRTR 0x0010 /* Mark the table as write through */
#define VM_DIR_USRP 0x0020 /* Mark the table as a user page */
#define VM_DIR_KRNP 0x0040 /* Mark the table as a kernel page  */
#define VM_DIR_READ 0x0080 /* Mark the table as readable */
#define VM_DIR_WRIT 0x0100 /* Mark the table as writable */
#define VM_DIR_PRES 0x0200 /* Mark the table as present */
#define VM_DIR_LRGP 0x0400 /* Specify the page size as large */
#define VM_DIR_SMLP 0x0800 /* Specify the page size as small */

/* GENERIC TABLE FLAGS */
#define VM_TBL_GLBL 0x0001 /* Mark the page as a global page */
#define VM_TBL_DRTY 0x0002 /* Mark the page as dirty */
#define VM_TBL_ACSS 0x0004 /* Mark the page as accessed */
#define VM_TBL_CACH 0x0008 /* Mark the page as not cached */
#define VM_TBL_WRTH 0x0010 /* Mark the page as write through */
#define VM_TBL_USRP 0x0020 /* Mark the page as a user page */
#define VM_TBL_KRNP 0x0040 /* Mark the page as a kernel page  */
#define VM_TBL_READ 0x0080 /* Mark the page as readable */
#define VM_TBL_WRIT 0x0100 /* Mark the page as writable */
#define VM_TBL_EXEC 0x0200 /* Mark the page as executable */
#define VM_TBL_PRES 0x0400 /* Mark the page as present */

#ifndef __VM_ASM_ONLY__

/* Include setup header */
#include "vm/vm_setup.h"

/* Include alloc header */
#include "vm/vm_alloc.h"

void vm_enable_paging(pgdir* dir);
pgdir* vm_curr_pgdir(void);
void vm_disable_paging(void);

/**
 * Save the current page directory and disable interrupts on this cpu.
 */
pgdir* vm_push_pgdir(void);

/**
 * Restore the previous page directory and pop_cli.
 */
void vm_pop_pgdir(pgdir* dir);

/**
 * Setup the kernel portion of the page table.
 */
void setup_kvm(void);

/**
 * Map the page at virtual address virt, to the physical address phy into the
 * page directory dir. This will create entries and tables where needed.
 */
int vm_mappage(uintptr_t phy, uintptr_t virt, pgdir* dir, 
	pgflags_t dir_flags, pgflags_t tbl_flags);

/**
 * Maps pages from va to sz. If certain pages are already mapped, they will be
 * ignored. 
 */
int vm_mappages(uintptr_t va, size_t sz, pgdir* dir, 
	pgflags_t dir_flags, pgflags_t tbl_flags);

/**
 * Directly map the pages into the page table.
 */
int vm_dir_mappages(uintptr_t start, uintptr_t end, pgdir* dir, 
	pgflags_t dir_flags, pgflags_t tbl_flags);

/**
 * Unmap the page from the page directory. If there isn't a page there then
 * nothing is done. Returns the page that was unmapped.
 */
uintptr_t vm_unmappage(uintptr_t virt, pgdir* dir);

/**
 * Find a page in a page directory. If the page is not mapped, and create is 1,
 * add a new page to the page directory and return the new address. If create
 * is 0, and the page is not found, return 0. Otherwise, return the address
 * of the mapped page.
 */
uintptr_t vm_findpg(uintptr_t virt, int create, pgdir* dir,
	pgflags_t dir_flags, pgflags_t tbl_flags);


pgflags_t vm_findtblflags(uintptr_t virt, pgdir* dir);
/**
 * Returns the flags for a virtual memory address. Returns -1 if the
 * page is not mapped in memory. 
 */
int findpgflags(uint virt, pgdir* dir);

int vm_pgflags(uintptr_t virt, pgdir* dir, pgflags_t tbl_flags);

/**
 * Sets the page read only. Returns the virtual address on success, 0 on
 * failure.
 */
int vm_pgreadonly(uintptr_t virt, pgdir* dir);

/**
 * Set the range of pages from virt_start to virt_end to read only.
 * returns 0 on success, 1 on failure.
 */
int vm_pgsreadonly(uintptr_t start, uintptr_t end, pgdir* dir);

/**
 * Free all pages in the page table and free the directory itself.
 */
void freepgdir(pgdir* dir);

/**
 * Copy the kernel portion of the page table
 */
int vm_copy_kvm(pgdir* dir);

/**
 * Instead of creating a copy of a user virtual memory space, directly map
 * the user pages.
 */
void vm_map_uvm(pgdir* dst_dir, pgdir* src_dir);

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
 * Move memory from one address space to another. Return the amount of bytes
 * copied.
 */
size_t vm_memmove(void* dst, const void* src, size_t sz,
                pgdir* dst_pgdir, pgdir* src_pgdir,
                pgflags_t dir_flags, pgflags_t tbl_flags);

/**
 * Free the directory and page table struct  but none of the pages pointed to 
 * by the tables.
 */
void freepgdir_struct(pgdir* dir);

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
//void pg_cmp(uchar* pg1, uchar* pg2);

#endif 

#endif
