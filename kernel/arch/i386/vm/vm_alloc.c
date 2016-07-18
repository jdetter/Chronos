/**
 * Author: John Detter <john@detter.com>
 *
 * Virtual memory manager (page allocator).
 */

#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "x86.h"
#include "cpu.h"
#include "file.h"
#include "stdlock.h"
#include "stdarg.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "k/vm.h"
#include "panic.h"

// #define DEBUG

#define KVM_MAGIC 0x55AA55AA

/* Free node template for the free list */
struct vm_free_node
{
	vmpage_t next; 
	int magic;
};

static int k_start_pages; /* How many pages did the vm start with? */
static int k_pages; /* How many pages are left? */
static struct vm_free_node* head; /* Start of the free list */

slock_t global_mem_lock; /* memory lock for free page list */

void vm_alloc_init(void)
{
	k_start_pages = 0;
	k_pages = 0;
	head = NULL;
	slock_init(&global_mem_lock);
}

vmpage_t palloc(void)
{
	slock_acquire(&global_mem_lock);
	pgdir_t* save = vm_push_pgdir();
        if(head == NULL) panic("No more free pages");
       	k_pages--;
        vmpage_t addr = (vmpage_t)head;
        if(head->magic != (int)KVM_MAGIC)
                panic("KVM is currupt!\n");
	if(addr < PGSIZE)
		panic("FVM is currupt! - NULL\n");

        head = (struct vm_free_node*)head->next;
        memset((void*)addr, 0, PGSIZE);

#ifdef DEBUG
        cprintf("Page allocated: 0x%x\n", addr);
#endif

	vm_pop_pgdir(save);
	slock_release(&global_mem_lock);
        return addr;
}

void pfree(vmpage_t pg)
{
	pg = PGROUNDDOWN(pg);

#ifdef DEBUG
	if(!pg) panic("Freed null page!!\n");
#endif
	if(!pg) return;
	slock_acquire(&global_mem_lock);
	pgdir_t* save = vm_push_pgdir();
        k_pages++;
	/* Make sure that the page doesn't have any flags: */

#ifdef __ALLOW_VM_SHARE__
	/* Was this page shared? */
	if(vm_pgunshare((pypage_t)pg))
	{
		slock_release(&global_mem_lock);
		return; /* Something still needs this page */
	}
#endif

	pg = PGROUNDDOWN(pg);
        struct vm_free_node* new_free = (struct vm_free_node*)pg;
        new_free->next = (vmpage_t)head;
        new_free->magic = (int)KVM_MAGIC;
        head = new_free;
	vm_pop_pgdir(save);
	slock_release(&global_mem_lock);

#ifdef DEBUG
        cprintf("Page freed: 0x%x\n", pg);
#endif
}

void vm_alloc_save_state(void)
{
        *(struct vm_free_node**)KVM_POOL_PTR = head;
        *(int*)KVM_PAGE_CT = k_pages;
        *(int*)KVM_VMODE = (int)((*(const uint16_t*)0x410) & 0x30);
}

void vm_alloc_restore_state(void)
{
        /* The kernel is now loaded, setup everything we need. */
        k_pgdir = (pgdir_t*)KVM_KPGDIR;
        head = *(struct vm_free_node**)KVM_POOL_PTR;
        k_pages = *(int*)KVM_PAGE_CT;
        video_mode = *(int*)KVM_VMODE;
        k_start_pages = k_pages;
}
