#ifndef _CONTEXT_SWITCH_H_
#define _CONTEXT_SWITCH_H_

/**
 * Switch to a user's page table and restore the context.
 */
void switch_uvm(struct proc* p);

/**
 * Switch to the user's context.
 */
void switch_context(struct proc* p);

#endif
