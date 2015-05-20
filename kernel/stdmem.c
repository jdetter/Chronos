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
	if(!mem_init) minit();

	return NULL;
}

int mfree(void* ptr)
{
	if(!mem_init) minit();

	return 0;
}

void minit(uint start_addr, uint end_addr, uint mem_map)
{
	mem_init = 1;
}
