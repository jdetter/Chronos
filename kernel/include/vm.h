#ifndef _VM_H_
#define _VM_H_

/* Include the architecture independant header */
#ifndef __NO_VM_ARCH__

#ifdef ARCH_i386
#include  "../arch/i386/include/vm.h"
#elif defined ARCH_x86_64
#include "../arch/x86_64/vm/vm.h"
#else
#error "Invalid architecture selected."
#endif

#endif /** #ifdef __NO_ARCH__ */

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

#ifdef __ALLOW_VM_SHARE__
#define VM_TBL_SHAR 0x1000 /* Mark the page as shared */
#define VM_TBL_COWR 0x2000 /* Mark the page as copy on write */
#endif

#ifndef __ASM_ONLY__

#include <stdint.h>
#include <sys/types.h>


/**
 * Initlize the memory manager. Returns 0 on success, non zero otherwise.
 */
extern int vm_init(void);

/**
 * Returns the physical address of the page directory currently in use
 */
extern pgdir_t* vm_curr_pgdir(void);

/**
 * Map the page at virtual address virt, to the physical address phy into the
 * page directory dir. This will create entries and tables where needed.
 */
extern int vm_mappage(pypage_t phy, vmpage_t virt, pgdir_t* dir, 
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Maps pages from va to sz. If certain pages are already mapped, they will be
 * ignored. Returns 0 on success, -1 otherwise.
 */
extern int vm_mappages(vmpage_t va, size_t sz, pgdir_t* dir, 
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Directly map the pages into the page table. Returns 0 on success, non
 * zero otherwise.
 */
extern int vm_dir_mappages(vmpage_t start, vmpage_t end, pgdir_t* dir, 
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Unmap the page from the page directory. If there isn't a page there then
 * nothing is done. Returns the page that was unmapped. Returns 0 if there
 * was no page unmapped.
 */
extern vmpage_t vm_unmappage(vmpage_t virt, pgdir_t* dir);

/**
 * Copy the contents of the src page into the dst page. Returns 0 on success.
 */
extern int vm_cpy_page(pypage_t dst, pypage_t src);

/**
 * Find a page in a page directory. If the page is not mapped, and create is 1,
 * add a new page to the page directory and return the new address. If create
 * is 0, and the page is not found, return 0. Otherwise, return the address
 * of the mapped page.
 */
extern pypage_t vm_findpg(vmpage_t virt, int create, pgdir_t* dir,
	vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Get the flags for the table that this page lives in.
 */
extern vmflags_t vm_findtblflags(vmpage_t virt, pgdir_t* dir);

/**
 * Get the flags for the directory that this page lives in. 
 */
extern vmflags_t vm_finddirflags(vmpage_t virt, pgdir_t* dir);

/**
 * Returns the flags for a virtual memory address. Returns -1 if the
 * page is not mapped in memory. 
 */
extern vmflags_t vm_findpgflags(vmpage_t virt, pgdir_t* dir);

/**
 * Set the flags for a page in a page table. Returns 0 on success.
 */
extern int vm_setpgflags(vmpage_t virt, pgdir_t* dir, vmflags_t pg_flags);

/**
 * Sets the page read only. Returns the virtual address on success, 0 on
 * failure.
 */
extern int vm_pgreadonly(vmpage_t virt, pgdir_t* dir);

/**
 * Set the range of pages from virt_start to virt_end to read only.
 * returns 0 on success, 1 on failure.
 */
extern int vm_pgsreadonly(vmpage_t start, vmpage_t end, pgdir_t* dir);

/**
 * Free all pages in the page table and free the directory itself.
 */
extern void freepgdir(pgdir_t* dir);

/**
 * Copy the kernel portion of the page table
 */
extern int vm_copy_kvm(pgdir_t* dir);

/**
 * Instead of creating a copy of a user virtual memory space, directly map
 * the user pages.
 */
extern void vm_map_uvm(pgdir_t* dst_dir, pgdir_t* src_dir);

/**
 * Make an exact copy of the user virtual memory space.
 */
extern void vm_copy_uvm(pgdir_t* dst, pgdir_t* src);

/**
 * Free the user portion of a page directory.
 */
extern void vm_free_uvm(pgdir_t* dir);

/**
 * Map another process's kstack into another process's address space.
 */
extern void vm_set_user_kstack(pgdir_t* dir, pgdir_t* kstack);

/**
 * Copy the kernel stack from src_dir and put it into dst_dir.
 */
extern void vm_cpy_user_kstack(pgdir_t* dst_dir, pgdir_t* src_dir);

/**
 * Map another process's stack into a process's stack swap space.
 */
extern void vm_set_swap_stack(pgdir_t* dir, pgdir_t* swap);

/**
 * Clear this directory's stack swap space.
 */
extern void vm_clear_swap_stack(pgdir_t* dir);

/**
 * Move memory from one address space to another. Return the amount of bytes
 * copied.
 */
extern size_t vm_memmove(void* dst, const void* src, size_t sz,
                pgdir_t* dst_pgdir, pgdir_t* src_pgdir,
                vmflags_t dir_flags, vmflags_t tbl_flags);

/**
 * Free the directory and page table struct  but none of the pages pointed to 
 * by the tables.
 */
extern void vm_freepgdir_struct(pgdir_t* dir);

/** Memory debugging functions */

/**
 * Print each mapping and flags in the page table. Returns the amount of
 * pages found in the page table.
 */
extern int vm_print_pgdir(vmpage_t last_page, pgdir_t* dir);

#ifdef __ALLOW_VM_SHARE__

/**
 * Initilize global structures for sharing memory on boot. Returns 0
 * on success, -1 otherwise.
 */
extern int vm_share_init(void);

/**
 * Share the given virtual page in the given address space. Returns 0 on
 * success. Returns -1 otherwise.
 */
extern int vm_pgshare(vmpage_t page, pgdir_t* pgdir);

/**
 * Check to see if the given page is shared. Returns 1 if the page is
 * shared, otherwise 0 is returned. 0 will also be returned if the
 * page doesn't exist.
 */
extern int vm_isshared(vmpage_t page, pgdir_t* pgdir);

/**
 * Unshare the page. This decrements the reference count for this page.
 * Returns the amount of references left to the page.
 */
extern int vm_pgunshare(pypage_t page);

/**
 * Share pages from base to base + sz. Returns 0 on success, -1 on failure.
 */
extern int vm_pgsshare(vmpage_t base, size_t sz, pgdir_t* pgdir);

/**
 * DEBUG FUNCTION: print the table and hashmap.
 */
extern void vm_share_print(void);

/**
 * Mark the user pages as copy on write. Returns 0 on success, -1 on failure.
 */
extern int vm_uvm_cow(pgdir_t* dir);

/**
 * Check to see if the given page is marked as copy on write. Returns 0 if
 * the page is not copy on write, returns 1 if the page is copy on write. This
 * function can safely be called on the same address space more than once.
 */
extern int vm_is_cow(pgdir_t* dir, vmpage_t page);

/**
 * Remove the copy on write property from the page. This will also allocated
 * a new page (if needed) and copy over the data. If this is the last reference
 * to the page, the page will just be marked as writable. The result of this
 * function will always be a page at the given address that is writable. Returns
 * 0 on success, nonzero otherwise.
 */
extern int vm_uncow(pgdir_t* dir, vmpage_t page);

#endif /* #ifdef __ALLOW_VM_SHARE__ */


/**
 * Allocate a page. Return NULL if there are no more free pages. The address
 * returned should be page aligned.
 * Security: Remember to zero the page before returning the address.
 */
extern pypage_t palloc(void);

/**
 * Free the page, pg. Add it to anywhere in the free list (no coalescing).
 */
extern void pfree(pypage_t pg);

#endif /* #ifndef __ASM_ONLY__*/

#endif
