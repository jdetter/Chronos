#ifndef _TRAP_H_
#define _TRAP_H_

/* Include machine dependant header */
#ifdef ARCH_i386
#include "arch/i386/trap/trap.h"
#else
#error "Invalid architecture selected."
#endif

#endif
