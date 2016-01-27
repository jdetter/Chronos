/**
 * Author: John Detter <john@detter.com>
 *
 * Virtual memory manager (page allocator).
 */

#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "kern/stdlib.h"
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
#include "panic.h"

#ifdef _ALLOW_VM_SHARE_
#define DEBUG
#endif
#define KVM_MAGIC 0x55AA55AA

struct kvm_region {vmpage_t start_addr; vmpage_t end_addr;};
struct vm_free_node{vmpage_t next; int magic;};

#define E820_UNUSED     0x01
#define E820_RESERVED   0x02
#define E820_ACPI_REC   0x03
#define E820_ACPI_NVS   0x04
#define E820_CURRUPT    0x05

#define E820_MAP_START  0x500

/**
 * Memory map entries from BIOS call int 15h, eax e820h
 */
struct e820_entry
{
        uint32_t addr_low;
        uint32_t addr_high;
        uint32_t length_low;
        uint32_t length_high;
        uint32_t type;
        uint32_t acpi_attr;
};

#define VM_IGNORE_TABLE_COUNT 0x04
struct kvm_region vm_ignore_table[] = {
        {PGROUNDDOWN(UVM_KVM_S), PGROUNDUP(UVM_KVM_E)}, /* Kernel mapping */
	/* Boot strap location */
	{PGROUNDDOWN(KVM_BOOT2_S), PGROUNDUP(KVM_BOOT2_E)},
        {0x00000000, 0x00002000} /* Null page + k_pgdir_t*/
};

int k_start_pages; /* How many pages did the vm start with? */
int k_pages; /* How many pages are left? */
pgdir_t* k_pgdir; /* Kernel page directory */
static struct vm_free_node* head;
int video_mode; /* The video mode during bootup */

int __check_paging__(void);
void __enable_paging__(pgdir_t* pgdir);
// pgdir_t* __get_cr3__(void);
static void vm_add_page(vmpage_t pg, pgdir_t* dir);
slock_t global_mem_lock;

void vm_stable_page_pool(void)
{
	struct e820_entry* entry = (void*)0x500;
	entry->addr_low =    0x00000000;
	entry->addr_high =   0x00000000;
	entry->length_low =  0x9fc00;
	entry->length_high = 0x00000000;
	entry->type = 1;
	entry->acpi_attr = 1;
	entry++;
	entry->addr_low =    0x100000;
	entry->addr_high =   0x00000000;
	entry->length_low =  0x10000000;
	//entry->length_low =  0x1fffe000;
	entry->length_high = 0x00000000;
	entry->type = 1;
	entry->acpi_attr = 1;
	entry++;
	memset(entry, 0, sizeof(struct e820_entry));
}

void vm_init_page_pool(void)
{
	slock_init(&global_mem_lock);
	k_pages = 0;
	head = NULL;
	k_pgdir = (pgdir_t*)KVM_KPGDIR;

        /* Get the information from the memory map. */
        int x = E820_MAP_START;
        for(;;x += sizeof(struct e820_entry))
        {
                struct e820_entry* e = (struct e820_entry*)x;
                /* Is this the end of the list? */
                if(e->type == 0x00) break;
                /* look at the type first, make sure it's ok to use. */
                if(e->type != E820_UNUSED) continue;

                /* The entry is free to use! */
                vmpage_t addr_start = e->addr_low;
                vmpage_t addr_end = addr_start + e->length_low;

		if(addr_start == 0x0) addr_start += 0x1000;
                vm_add_pages(addr_start, addr_end, k_pgdir);
        }
	
}

/* Take pages from start to end and add them to the free list. */
void vm_add_pages(vmpage_t start, vmpage_t end, pgdir_t* dir)
{
        /* Make sure the start and end are page aligned. */
        start = PGROUNDDOWN((start + PGSIZE - 1));
        end = PGROUNDDOWN(end);
        /* There must be at least one page. */
        if(end - start < PGSIZE) return;
        int x;
        for(x = start;x != end;x += PGSIZE) vm_add_page(x, dir);
}

static void vm_add_page(vmpage_t pg, pgdir_t* dir)
{
	/* Should we ignore this page? */
        int x;
        for(x = 0;x < VM_IGNORE_TABLE_COUNT;x++)
        {
                struct kvm_region* m = vm_ignore_table + x;
                if(pg >= m->start_addr && pg < m->end_addr) return;
        }

        pfree(pg);
	/* If there is a dir, map this page into the dir */
	if(dir) vm_dir_mappages(pg, pg + PGSIZE, dir, 
		VM_DIR_WRIT, VM_TBL_WRIT);
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
#ifdef DEBUG
        cprintf("Page freed: 0x%x\n", pg);
#endif
        k_pages++;
	/* Make sure that the page doesn't have any flags: */

#ifdef _ALLOW_VM_SHARE_
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
}

void setup_kvm(void)
{
	/* The kernel is now loaded, setup everything we need. */
	k_pgdir = (pgdir_t*)KVM_KPGDIR;
	head = *(struct vm_free_node**)KVM_POOL_PTR;
	k_pages = *(int*)KVM_PAGE_CT;
	video_mode = *(int*)KVM_VMODE;
	k_start_pages = k_pages;
}

void vm_save_vm(void)
{
	*(struct vm_free_node**)KVM_POOL_PTR = head;
	*(int*)KVM_PAGE_CT = k_pages;
	*(int*)KVM_VMODE = (int)((*(const uint16_t*)0x410) & 0x30);
}
