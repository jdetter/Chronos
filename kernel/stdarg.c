/**
 * Author: John Detter <john@detter.com>
 *
 * Functions for handling variable argument lists. This will
 * eventually be replaced by all macros.
 */

#include "types.h"
#include "x86.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"

void va_start(va_list* list, void** first_arg)
{
	memset(list, 0, sizeof(va_list));
	list->first_arg = first_arg;
}

void* va_arg(va_list* list, int position)
{
	void** first_arg = list->first_arg + position;
	return *first_arg;
}

void va_end(va_list* list)
{
	memset(list, 0, sizeof(va_list));
}
