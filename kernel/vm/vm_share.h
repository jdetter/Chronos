#ifndef _VM_SHARE_H_
#define _VM_SHARE_H_

#include "vm.h"

/**
 * Initilize global structures for sharing memory on boot. Returns 0
 * on success, -1 otherwise.
 */
int vm_share_init(void);

/**
 * Allow the given physical page to be shared. Returns 0 on success,
 * -1 otherwise.
 */
int vm_pgshare(pypage_t pypage);

/**
 * Returns 1 if the physical page is shared. Returns 0 if the page doesn't
 * exist or it isn't shared.
 */
int vm_isshared(pypage_t pypage);

/**
 * Unshare the page. This decrements the reference count for this page.
 * Returns the amount of references left to the page.
 */
int vm_pgunshare(pypage_t pypage);

#endif
