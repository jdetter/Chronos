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
#define VM_TBL_PRES 0x0200 /* Mark the page as present */
#define VM_TBL_EXEC 0x0400 /* Mark the page as executable */
#define VM_TBL_SHAR 0x1000 /* Mark the page as shared */
#define VM_TBL_COWR 0x2000 /* Mark the page as copy on write */

#ifndef __VM_ASM_ONLY__

#include <stdint.h>
#include <sys/types.h>

/* Include setup header */
#include "vm/vm_setup.h"

/* Include alloc header */
#include "vm/vm_alloc.h"

#ifdef _ALLOW_VM_SHARE_
#include "vm/vm_share.h"
#endif

void vm_enable_paging(pgdir_t* dir);
pgdir_t* vm_curr_pgdir(void);
void vm_disable_paging(void);

/**
 * Save the current page directory and disable interrupts on this cpu.
 */
pgdir_t* vm_push_pgdir(void);

/**
 * Restore the previous page directory and pop_cli.
 */
void vm_pop_pgdir(pgdir_t* dir);

/**
 * Setup the kernel portion of the page table.
 */
void setup_kvm(void);

/**
 * Map the page at virtual address virt, to the physical address phy into the
 * page directory dir. This will create entries and tables where needed.
 */
int vm_mappage(pypage_t phy, vmpage_t virt, pgdir_t* dir, 
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Maps pages from va to sz. If certain pages are already mapped, they will be
 * ignored. Returns 0 on success, -1 otherwise.
 */
int vm_mappages(vmpage_t va, size_t sz, pgdir_t* dir, 
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Directly map the pages into the page table.
 */
int vm_dir_mappages(vmpage_t start, vmpage_t end, pgdir_t* dir, 
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Unmap the page from the page directory. If there isn't a page there then
 * nothing is done. Returns the page that was unmapped. Returns 0 if there
 * was no page unmapped.
 */
vmpage_t vm_unmappage(vmpage_t virt, pgdir_t* dir);

/**
 * Find a page in a page directory. If the page is not mapped, and create is 1,
 * add a new page to the page directory and return the new address. If create
 * is 0, and the page is not found, return 0. Otherwise, return the address
 * of the mapped page.
 */
vmpage_t vm_findpg(vmpage_t virt, int create, pgdir_t* dir,
	vmflags_t dir_flags, vmflags_t tbl_flags);


vmflags_t vm_findtblflags(vmpage_t virt, pgdir_t* dir);
/**
 * Returns the flags for a virtual memory address. Returns -1 if the
 * page is not mapped in memory. 
 */
vmflags_t vm_findpgflags(vmpage_t virt, pgdir_t* dir);

/**
 * Set the flags for a page in a page table.
 */
int vm_setpgflags(vmpage_t virt, pgdir_t* dir, vmflags_t pg_flags);

/**
 * Sets the page read only. Returns the virtual address on success, 0 on
 * failure.
 */
int vm_pgreadonly(vmpage_t virt, pgdir_t* dir);

/**
 * Set the range of pages from virt_start to virt_end to read only.
 * returns 0 on success, 1 on failure.
 */
int vm_pgsreadonly(vmpage_t start, vmpage_t end, pgdir_t* dir);

/**
 * Free all pages in the page table and free the directory itself.
 */
void freepgdir(pgdir_t* dir);

/**
 * Copy the kernel portion of the page table
 */
int vm_copy_kvm(pgdir_t* dir);

/**
 * Instead of creating a copy of a user virtual memory space, directly map
 * the user pages.
 */
void vm_map_uvm(pgdir_t* dst_dir, pgdir_t* src_dir);

/**
 * Make an exact copy of the user virtual memory space.
 */
void vm_copy_uvm(pgdir_t* dst, pgdir_t* src);

/**
 * Free the user portion of a page directory.
 */
void vm_free_uvm(pgdir_t* dir);

/**
 * Use the kernel's page table
 */
void switch_kvm(void);


/**
 * Map another process's kstack into another process's address space.
 */
void vm_set_user_kstack(pgdir_t* dir, pgdir_t* kstack);

/**
 * Map another process's stack into a process's stack swap space.
 */
void vm_set_swap_stack(pgdir_t* dir, pgdir_t* swap);

/**
 * Clear this directory's stack swap space.
 */
void vm_clear_swap_stack(pgdir_t* dir);

/**
 * Move memory from one address space to another. Return the amount of bytes
 * copied.
 */
size_t vm_memmove(void* dst, const void* src, size_t sz,
                pgdir_t* dst_pgdir, pgdir_t* src_pgdir,
                vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Free the directory and page table struct  but none of the pages pointed to 
 * by the tables.
 */
void vm_freepgdir_struct(pgdir_t* dir);

/** Memory debugging functions */

/**
 * Print each mapping and flags in the page table. Returns the amount of
 * pages found in the page table.
 */
int vm_print_pgdir(vmpage_t last_page, pgdir_t* dir);

#endif 

#endif
