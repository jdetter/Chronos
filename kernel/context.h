#ifndef _CONTEXT_H_
#define _CONTEXT_H_

/* Include architecture context switching header */
#ifdef ARCH_i386
#include  "arch/i386/context/context-switch.h"
#else
#error "Invalid architecture selected."
#endif

#endif
