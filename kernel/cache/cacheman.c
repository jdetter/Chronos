#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "stdlock.h"
#include "file.h"
#include "pipe.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"


/**
 * Represents a cache node linked list.
 */
struct cman_node
{
	size_t sz; /* Size of this node */
	struct cman_node* next; /* Next node in the list */
};

static struct cman_node* head;

void cman_init(void)
{
	/* Zero our space */
	size_t sz = PGROUNDUP(KVM_DISK_E - KVM_DISK_S);
	memset((void*)KVM_DISK_S, 0, sz);
	head = (void*)KVM_DISK_S;
	head->sz = sz;
}

void* cman_alloc(size_t sz)
{
	if(sz == 0) return NULL;

	sz = PGROUNDUP(sz);
	struct cman_node* curr = head;
	struct cman_node* last = NULL;
	struct cman_node* result = NULL;

	/* Search for a node with enough room */
	while(curr)
	{
		if(curr->sz >= sz)
		{
			result = curr;
			break;
		}
		last = curr;
		curr = curr->next;
	}

	/* Did we find anything? */
	if(!result)
	{
		cprintf("cman: Disk cache out of space!\n");
		return NULL;
	}

	if(sz == result->sz)
	{
		/* Consume the node */
		if(last)
		{
			/* update the head */
			head = result->next;
		} else {
			/* Update the broken pointer */
			last->next = result->next;
		}
	} else {
		/* Partition this node */
		
		curr = result;
		/* Assign result */
		result = (struct cman_node*)(((char*)curr) + sz);
		/* Modify the size */
		curr->sz -= sz;
	}

	result->next = NULL;
	return (void*)result;
}

void cman_free(void* ptr, size_t sz){}
