#include "types.h"
#include "x86.h"
#include "stdmem.h"

#define M_AMT 0x5000 /* 5K heap space*/
#define M_MAGIC (void*)(0x43524E53)
int mem_init = 0;

/* A node in the free list. */
typedef struct free_node
{
	int sz; /* The size available here (not including this header) */
	struct free_node* next; /* The next free header in the list.*/
} free_node;

/* An allocated node. */
typedef struct alloc_node
{
	int sz; /* The size of this allocated region. */
	void* magic; /* Make sure this node isn't currupted. */
} alloc_node;

struct free_node* head; /* The head of the free list */
struct free_node* curr; /* Pointer to the current location. */

void* malloc(uint sz)
{
	if(!mem_init) minit(0, 0, 1);

	if(head == NULL) return NULL;
	if(sz == 0) return NULL;

	/* 
	 * If we get back to start_search, we know that we have 
	 * looped through the list and we should stop. 
	 */
	free_node* start_search = curr;
	int found = 0; /* Did we find a large enough node? */

	/*
	 * Search for a node in the free list that is large
	 * enough to service the user's request.
	 */
	do
	{
		/* If we just passed the last node, start at the beginning. */
		if(curr == NULL) curr = head;

		if(curr->sz >= sz)
		{
			/* We found a large enough node. */
			found = 1;
			break;
		}

		/* The current node is not large enough, iterate forward. */
		curr = curr->next;
	} while(curr != start_search);

	if(found == 0)
	{
		/* We don't have a node large enough to service the request. */
		return NULL;
	}

	/* We did find a large enough node */

	uint remaining_bytes = curr->sz - sz;
	if(remaining_bytes == 0)
	{
		alloc_node* new_block = curr;
		new_block->sz = sz;
		new_block->magic = M_MAGIC;
	} else {
			
	}

	/* Fix broken pointer */

	return NULL;
}

int mfree(void* ptr)
{
	if(!mem_init) minit(0, 0, 1);

	return 0;
}

void minit(uint start_addr, uint end_addr, uint mem_map)
{
	/* total_bytes is the amount of bytes we were given */
	uint total_bytes = end_addr - start_addr;
	/* Start of the free list were creating */
	free_node* start_list = (free_node*)start_addr;
	start_list->sz = total_bytes - sizeof(struct free_node);
	start_list->next = NULL; /* The only element in the list */

	head = start_list; /* Assign head */
	curr = start_list; /* Assign current free node */

	mem_init = 1; /* We have initilized the free list */
}
