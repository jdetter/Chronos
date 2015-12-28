#include "vm.h"

#if ((!defined _ARCH_SHARE_PROVIDED_) && (defined _ALLOW_VM_SHARE_))

#include <sys/types.h>
#include <string.h>
#include "stdlock.h"
#include "panic.h"

#define PARANOID
#define DEBUG
#define MAX_SHARE 16384
#define SHARE_HASHMAP 128

struct vm_share
{
	int valid; /* Whether or not this entry is in use. */
	pypage_t page; /* Physical address of the shared page */
	int refs; /* How many people point to this page? */
	struct vm_share* next; /* Next vm_share in the list */
};

typedef struct vm_share vm_share_t;

static slock_t share_table_lock;
static vm_share_t share_table[MAX_SHARE];
static int table_clock;

static vm_share_t* share_hashmap[SHARE_HASHMAP];

int vm_share_init(void)
{
        slock_init(&share_table_lock);
        memset(share_table, 0, sizeof(vm_share_t) * MAX_SHARE);
	memset(share_hashmap, 0, sizeof(vm_share_t*) * SHARE_HASHMAP);
        table_clock = 0;

        return 0;
}

static int vm_share_hash(vmpage_t page)
{
	if(!page) return 0;
	return (page >> PGSHIFT) & (SHARE_HASHMAP - 1);
}

/**
 * Insert this share object into the hashmap.
 */
static void vm_share_insert(vm_share_t* sh)
{
	slock_acquire(&share_table_lock);
	vmpage_t page = sh->page;
	page = PGROUNDDOWN(page);
	int index = vm_share_hash(page);

	if(!share_hashmap[index])
	{
		share_hashmap[index] = sh;
		slock_release(&share_table_lock);
		return;
	}

	/* Get the head */
	vm_share_t* pos = share_hashmap[index];

	/* Find the end */
	while(pos->next) pos = pos->next;

	/* Append this struct */
	pos->next = sh;

	/* Safety: zero the next pointer */
	sh->next = NULL;
	slock_release(&share_table_lock);
}

/**
 * Remove this share object from the hashmap.
 */
static void vm_share_remove(vm_share_t* sh)
{
	slock_acquire(&share_table_lock);
	pypage_t page = PGROUNDDOWN(sh->page);
	int index = vm_share_hash(page);

	vm_share_t* head = share_hashmap[index];

	if(head == sh)
	{
		/* we're removing the head */
		share_hashmap[index] = sh->next;
		slock_release(&share_table_lock);
		return;
	}

	/* Find the element before sh in the list */
	while(head && head->next != sh) 
		head = head->next;

	/* Did we find it? */
	if(!head) 
	{
		slock_release(&share_table_lock);
		return;
	}

	/* Set this next equal to sh's next */
	head->next = sh->next;
	slock_release(&share_table_lock);
}

/**
 * Lookup the page in the hashmap.
 */
static vm_share_t* vm_share_lookup(pypage_t page)
{
	slock_acquire(&share_table_lock);
	page = PGROUNDDOWN(page);
	int index = vm_share_hash(page);
	vm_share_t* pos = share_hashmap[index];

	while(pos && pos->page != page)
		pos = pos->next;

	slock_release(&share_table_lock);
	return pos;
}

static vm_share_t* vm_share_alloc(void)
{
	vm_share_t* result = NULL;

	slock_acquire(&share_table_lock);
	if(table_clock >= MAX_SHARE || table_clock < 0)
		table_clock = 0;
	int start = table_clock;
	do
	{
		/* Does this work? */
		if(!share_table[table_clock].valid)
			break;

		/* Increment clock */
		table_clock++;
		/* Do we need to wrap? */
		if(table_clock >= MAX_SHARE || table_clock < 0)
			table_clock = 0;
	} while(start != table_clock);

	if(!share_table[table_clock].valid)
	{
		/* Intilize this share struct */
		share_table[table_clock].valid = 1;
		share_table[table_clock].page = 0;
		share_table[table_clock].refs = 0;
		share_table[table_clock].next = NULL;
		result = share_table + table_clock;
	}

	slock_release(&share_table_lock);
	
	return result;
}

static void vm_share_free(vm_share_t* s)
{
	/* sanity check */
	if(!s || s < share_table || s + MAX_SHARE >= share_table) 
		return;

	slock_acquire(&share_table_lock);
	memset(s, 0, sizeof(vm_share_t));
	slock_release(&share_table_lock);
}

int vm_pgshare(vmpage_t page, pgdir_t* pgdir)
{
#ifdef DEBUG
	cprintf("vm_share: starting share for page 0x%x\n", page);
#endif

	slock_acquire(&share_table_lock);
	pypage_t py = vm_findpg(page, 0, pgdir, 0, 0);
	if(!py) 
	{
#ifdef DEBUG
		cprintf("vm_share: ERROR: page doesn't exist\n");
#endif
		/* This page doesn't exist. */
		slock_release(&share_table_lock);
		return -1;
	}

	vm_share_t* st = NULL;
	if((st = vm_share_lookup(py)) != NULL)
	{
#ifdef DEBUG
		cprintf("vm_share: Share already existed.\n");
#endif
		/* Just increment the ref count */
		st->refs++;

#ifdef DEBUG
		cprintf("vm_share: new amount of refs: %d\n", st->refs);
#endif
	} else {
#ifdef DEBUG
		cprintf("vm_share: Creating new share.\n");
#endif
		/* Allocate a new share struct */
		st = vm_share_alloc();

		/* Setup this struct */
		st->page = py;
		st->refs = 1;
		st->next = NULL;

		/* Add this st to the hash map */
		vm_share_insert(st);
	}

#ifdef DEBUG
	cprintf("vm_share: adjusting pgdir flags...\n");
#endif

	/* Adjust the page directory flags */
	vmflags_t flags = vm_findtblflags(page, pgdir);
	flags |= VM_TBL_SHAR;

	/* Set the page table flags */
	if(vm_setpgflags(page, pgdir, flags))
	{
#ifdef DEBUG
		cprintf("vm_share: ERROR: couldn't set flags!\n");
#endif
		slock_release(&share_table_lock);
		
		/* Because of the failure, decrement refs */
		st->refs--;
		return -1;
	}
	
#ifdef DEBUG
	cprintf("vm_share: share completed successfully.\n");
#endif

	slock_release(&share_table_lock);
	return 0;
}

int vm_isshared(vmpage_t page, pgdir_t* pgdir)
{
#ifdef DEBUG
	cprintf("vm_share: is page 0x%x shared?\n", page);
#endif

	slock_acquire(&share_table_lock);
	pypage_t py = vm_findpg(page, 0, pgdir, 0, 0);
	py = PGROUNDDOWN(py);

	if(!py) 
	{
#ifdef DEBUG
		cprintf("vm_share: page doesn't exist!\n");
#endif	
		slock_release(&share_table_lock);
		return 0;
	}

	vmflags_t flags = vm_findpgflags(page, pgdir);
	flags &= VM_TBL_SHAR;

#ifdef PARANOID
	if(!flags)
	{
#ifdef DEBUG
		cprintf("vm_share: starting paranoid check...\n");
#endif
		vm_share_t* st = vm_share_lookup(py);
		if(!st) 
		{
#ifdef DEBUG
			cprintf("vm_share: paranoid check passed.\n");
#endif
			slock_release(&share_table_lock);
			return 0;
		} else {
#ifdef DEBUG
			cprintf("vm_share: ERROR: paranoid check failed!\n");
#endif
			flags = 1;
		}
	}
#endif

#ifdef DEBUG
	cprintf("vm_share: Is page shared? %s\n", flags?"yes":"no");
#endif

	slock_release(&share_table_lock);
	return flags ? 1 : 0;
}

int vm_pgunshare(pypage_t py)
{
#ifdef DEBUG
	cprintf("vm_share: starting unshare for page 0x%x.\n", py);
#endif
	slock_acquire(&share_table_lock);
	py = PGROUNDDOWN(py);

	/* Get the entry from the hash table */
	vm_share_t* st = vm_share_lookup(py);
	if(!st)
	{
#ifdef DEBUG
		cprintf("vm_share: ERROR: page not in hashmap\n");
#endif
		slock_release(&share_table_lock);
		return -1;
	}

	/* decrement references */
	st->refs--;

#ifdef DEBUG
	cprintf("vm_share: new ref count: %d\n", st->refs);
#endif
	if(st->refs <= 0)
	{
		/* Unallocate this page */
		pfree(py);
		/* Remove this from the hash table */
		vm_share_remove(st);
		/* Free this st */
		vm_share_free(st);

#ifdef DEBUG
		cprintf("vm_share: share has been unallocated.\n");
#endif
	}

#ifdef DEBUG
	cprintf("vm_share: unshare complete!\n");
#endif

	slock_release(&share_table_lock);
	return 0;
}

int vm_pgsshare(vmpage_t base, size_t sz, pgdir_t* pgdir)
{
	vmpage_t pos;
	for(pos = base;pos < base + sz;pos += PGSIZE)
	{
		if(vm_pgshare(pos, pgdir))
			return 1;
	}
	
	return 0;
}

#endif
