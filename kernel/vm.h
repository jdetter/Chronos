#ifndef _UVM_H_
#define _UVM_H_

typedef uint* pgdir;
typedef uint* pgtbl;

#define PGROUNDDOWN(pg)	(pg & ~(PGSIZE - 1))	

#define KVM_START 	0x100000
#define KVM_END		0xEFFFFF
#define KVM_MALLOC	0x200000

/**
 * Initilize a free list of pages from address start to address end.
 */
uint vm_init(uint start, uint end);

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
 * page directory dir. This will create entries and tables where needed.
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
 * Use the kernel's page table
 */
void switch_kvm(void);

/**
 * Switch to a user's page table and resume execution.
 */
void switch_uvm(pgdir* dir);

#endif
