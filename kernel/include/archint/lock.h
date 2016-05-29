#ifndef _ARCHINT_LOCK_H_
#define _ARCHINT_LOCK_H_

#include "archint/architecture.h"

struct concurrency_funcs_t
{
	size_t (*fetch_and_add)(void* addr, int val);
	size_t (*compare_and_swap)(void* addr, int a, int b);
};

/**
 * Allows the architecture to tell the kernel which concurrency
 * functions are available and assigns a default.
 */
int archlock_init(struct concurrency_funcs_t* functions);

#endif
