#include "vm.h"

#if ((!defined _ARCH_SHARE_PROVIDED_) && (defined _ALLOW_VM_SHARE_))

#include <sys/types.h>
#include <string.h>
#include "stdlock.h"

#define MAX_SHARE 512

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
	s->valid = 0;
	slock_release(&share_table_lock);
}

int vm_share_init(void)
{
	slock_init(&share_table_lock);
	memset(share_table, 0, sizeof(vm_share_t) * MAX_SHARE);
	table_clock = 0;
	return 0;
}

int vm_pgshare(pypage_t pypage)
{
	vm_share_free(vm_share_alloc()); /* TODO: suppress */
	return 0;
}

int vm_isshared(pypage_t pypage)
{
	return 0;
}

int vm_pgunshare(pypage_t pypage)
{
	return 0;
}


#endif
