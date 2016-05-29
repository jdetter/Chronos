#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <stdint.h>

struct proc;

typedef uintptr_t context_t;


/**
 * Enter the process' context. This implies that the current cpu is running the
 * scheduler loop.
 */
void context_switch(struct proc* p);

/**
 * Enter the given context (new_context) and save the current context (old_context).
 * This function can be used to return to the process scheduler or to enter another
 * process' context.
 */
void context_restore(context_t* old_context, context_t new_context);

#endif /* #ifndef _CONTEXT_H_ */
