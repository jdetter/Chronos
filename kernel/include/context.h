#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#ifndef __ASM_ONLY__

#include <stdint.h>

typedef uintptr_t context_t;

struct proc;

void context_switch(struct proc* p);

void context_restore(context_t* old_context, context_t new_context);

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

#endif /* #ifndef __ASM_ONLY__ */

#endif /* #ifndef _CONTEXT_H_ */
