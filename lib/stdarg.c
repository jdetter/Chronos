#include "types.h"
#include "x86.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"

void va_start(va_list* list, void** first_arg)
{
	struct va_list_node* curr_ptr = NULL;
	struct va_list_node* head = NULL;
	first_arg++;

	while(*(first_arg))
	{
		struct va_list_node* next = 
			malloc(sizeof(struct va_list_node));
		memset(next, 0, sizeof(struct va_list_node));
		next->arg_ptr = (void*)first_arg;
		next->next = NULL;
		first_arg++;

		if(head == NULL)
		{
			head = curr_ptr = next;
			continue;
		}

		curr_ptr->next = next;
		curr_ptr = next;
	}

	*list = head;
}

void* va_arg(va_list list, int position)
{
	int x;
	for(x = 0;x < position;x++)
	{
		if(list == NULL) return NULL;
		list = list->next;
	}

	if(list == NULL) return NULL;
	return list->arg_ptr;
}

void va_end(va_list list)
{
	while(list)
	{
		mfree(list);
		list = list->next;
	}
}
