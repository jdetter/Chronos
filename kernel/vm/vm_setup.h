#ifndef _VM_SETUP_H_
#define _VM_SETUP_H_

/**
 * Initlize the memory manager. Returns 0 on success, non zero otherwise.
 */
int vm_init(void);

/**
 * Initilize segmentation if it is used. Returns 0 on success, non zero
 * otherwise.
 */
int vm_seg_init(void);

/**
 * Setup the page pool. This is done during the boot strap.
 */
void vm_init_page_pool(void);

/**
 * Add pages to the page pool. This will not add pages to the page pool
 * that are going to be direct mapped.
 */
void vm_add_pages(uint start, uint end, pgdir* dir);

/**
 * Called when the bootstrap has finished setting up memory. The
 * information saved by this function call is read by the kernel
 * when it starts executing.
 */
void vm_save_vm(void);

#endif
