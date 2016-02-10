#ifndef _VM_SHARE_H_
#define _VM_SHARE_H_

#include "vm.h"

/**
 * Initilize global structures for sharing memory on boot. Returns 0
 * on success, -1 otherwise.
 */
int vm_share_init(void);

/**
 * Share the given virtual page in the given address space. Returns 0 on
 * success. Returns -1 otherwise.
 */
int vm_pgshare(vmpage_t page, pgdir_t* pgdir);

/**
 * Check to see if the given page is shared. Returns 1 if the page is
 * shared, otherwise 0 is returned. 0 will also be returned if the
 * page doesn't exist.
 */
int vm_isshared(vmpage_t page, pgdir_t* pgdir);

/**
 * Unshare the page. This decrements the reference count for this page.
 * Returns the amount of references left to the page.
 */
int vm_pgunshare(pypage_t page);

/**
 * Share pages from base to base + sz. Returns 0 on success, -1 on failure.
 */
int vm_pgsshare(vmpage_t base, size_t sz, pgdir_t* pgdir);

/**
 * DEBUG FUNCTION: print the table and hashmap.
 */
void vm_share_print(void);

/**
 * Mark the user pages as copy on write. Returns 0 on success, -1 on failure.
 */
int vm_uvm_cow(pgdir_t* dir);

/**
 * Check to see if the given page is marked as copy on write. Returns 0 if
 * the page is not copy on write, returns 1 if the page is copy on write. This
 * function can safely be called on the same address space more than once.
 */
int vm_is_cow(pgdir_t* dir, vmpage_t page);

/**
 * Remove the copy on write property from the page. This will also allocated
 * a new page (if needed) and copy over the data. If this is the last reference
 * to the page, the page will just be marked as writable. The result of this
 * function will always be a page at the given address that is writable. Returns
 * 0 on success, nonzero otherwise.
 */
int vm_uncow(pgdir_t* dir, vmpage_t page);

#endif
