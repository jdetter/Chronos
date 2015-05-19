#include "types.h"
#include "x86.h"
#include "stdlib.h"
#include "stdarg.h"

void va_start(va_list* list, void** first_arg)
{
	struct va_list_node** curr_ptr = list;
	*curr_ptr = NULL;
	first_arg++;

	while(*(first_arg))
	{

	}
}

void* va_arg(va_list list, int arg)
{
	return NULL;
}
