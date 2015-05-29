#include "types.h"
#include "x86.h"
#include "vm.h"
#include "panic.h"

struct vm_free_node
{
	uint next;
};

struct vm_free_node* head;

uint vm_init(uint start, uint end)
{
	/* Make sure the start and end are page aligned. */
	start = PGROUNDDOWN(start);
	end = PGROUNDDOWN(end);

	/* There must be at least one page. */
	if(end - start < PGSIZE) return 0;
	
	/* Set the start of the list */
	head = (struct vm_free_node*)start;
	struct vm_free_node* curr = head;
	uint addr = start + PGSIZE; /* The address of the next page */
	for(addr = start;addr <= (end - PGSIZE);addr += PGSIZE)
	{
		curr->next = addr;
		curr = (struct vm_free_node*)(((uchar*)curr) + PGSIZE);
	}

	curr->next = 0; /* Set the end of the list */

	/* Return the amount of pages added to the list */
	return (end - start) / PGSIZE;
}

uint palloc(void)
{
	if(head == NULL) return 0;
	uint addr = (uint)head;
	head = (struct vm_free_node*)head->next;
	return addr;
}

void pfree(uint pg)
{
	if(pg == 0) 
	{
		panic("Free null page.\n");
	}
	struct vm_free_node* new_free = (struct vm_free_node*)pg;
	new_free->next = (uint)head;
	head = new_free;
}

void mappage(uint phy, uint virt, pgdir* dir)
{

}

uint findpg(uint virt, int create, pgdir* dir)
{
	return 0;
}

void freepgdir(pgdir* dir)
{

}

void switch_kvm(void)
{

}

void switch_uvm(pgdir* dir)
{

}
