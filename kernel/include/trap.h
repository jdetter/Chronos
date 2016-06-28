#ifndef _TRAP_H_
#define _TRAP_H_

/* Include machine dependant header */
#ifdef ARCH_i386
#include "../arch/i386/include/trap.h"
#elif defined ARCH_x86_64
#include "../arch/x86_64/include/trap.h"
#else
#error "Invalid architecture selected."
#endif

#ifndef __ASM_ONLY__

/**
 * Callback for when fork completes. This is where the child
 * process will start inside of the kernel.
 */
void tp_fork_return(void);

#endif

#endif
