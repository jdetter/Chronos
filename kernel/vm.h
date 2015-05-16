#ifndef _UVM_H_
#define _UVM_H_

/**
 * Initilize a free list of pages from address phy to (phy + sz).
 */
void vminit(uint phy, uint sz);

/**    
 * Allocate a page. Return NULL if there are no more free pages. The address
 * returned should be page aligned.
 * Security: Remember to zero the page before returning the address.
 */
uint palloc(void);

/**
 * Free the page, pg. Add it to anywhere in the free list (no coalescing).
 */
void pfree(uint pg);

/**
 * Map the page at virtual address virt, to the physical address phy into the
 * page directory dir.
 */
void mappage(uint phy, uint virt, pgdir* dir);

/**
 * Find a page in a page directory. If the page is not mapped, and create is 1,
 * add a new page to the page directory and return the new address. If create
 * is 0, and the page is not found, return 0. Otherwise, return the address
 * of the mapped page.
 */
uint findpg(uint virt, int create, pgdir* dir);

/**
 * Free all pages in the page table and free the directory itself.
 */
void freepgdir(pgdir* dir);

/**
 * Enable paging and set the page table base register to dir.
 */
void pgenable(pgdir* dir);

/**
 * Disable paging and clear the page table base register.
 */
void pgdisable(pgdir* dir);

#endif
