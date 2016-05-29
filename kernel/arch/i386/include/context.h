#ifndef _ARCH_CONTEXT_H
#define _ARCH_CONTEXT_H

#include "k/context.h"

struct context
{
        uint32_t cr0; /* Saved page table base register */

        uint32_t edi; /* pushal */
        uint32_t esi;
        uint32_t ebp;
        uint32_t esp;
        uint32_t ebx;
        uint32_t edx;
        uint32_t ecx;
        uint32_t eax;

        uint32_t eip;
};

#endif
