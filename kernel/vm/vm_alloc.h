#ifndef _VM_ALLOC_H_
#define _VM_ALLOC_H_

#include <stdint.h>
#include "vm.h"

/**    
 * Allocate a page. Return NULL if there are no more free pages. The address
 * returned should be page aligned.
 * Security: Remember to zero the page before returning the address.
 */
pypage_t palloc(void);

/**
 * Free the page, pg. Add it to anywhere in the free list (no coalescing).
 */
void pfree(pypage_t pg);

#endif
