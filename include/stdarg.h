#ifndef _STDARG_H_
#define _STDARG_H_

/* A node in the variable arguments list */
struct va_list_node
{
	void* arg_ptr; /* A pointer to the argument */
	struct va_list_node* next; /* The next argument */
};
typedef struct va_list_node* va_list;

/**
 * Create a list of variable arguments.
 */
void va_start(va_list* list, void** first_arg);

/**
 * Get the argument in the list at the position.
 */
void* va_arg(va_list list, int position);

/**
 * Free an argument list.
 */
void va_end(va_list list);

#endif
