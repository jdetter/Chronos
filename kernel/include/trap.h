#ifndef _TRAP_H_
#define _TRAP_H_

/* Include machine dependant header */
#include "arch/trap.h"

/**
 * Callback for when fork completes. This is where the child
 * process will start inside of the kernel.
 */
void tp_fork_return(void);

#endif
