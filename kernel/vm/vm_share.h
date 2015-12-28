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

#endif
