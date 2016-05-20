#ifndef _TRAP_H_
#define _TRAP_H_

/* Include machine dependant header */
#ifdef ARCH_i386
#include "arch/i386/trap/trap.h"
#elif defined ARCH_x86_64
#include "arch/x86_64/trap/trap.h"
#else
#error "Invalid architecture selected."
#endif

#endif
