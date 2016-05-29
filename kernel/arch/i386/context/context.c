#include "context.h"
#include "proc.h"
#include "x86.h"
#include "trap.h"
#include "panic.h"

extern context_t k_context;
extern struct vm_segment_descriptor global_descriptor_table[];

void context_switch(struct proc* p)
{
        /* Set the task segment to point to the process's stacks. */
        uintptr_t base = (uint)p->tss;
        uintptr_t limit = sizeof(struct task_segment);
        unsigned int type = TSS_DEFAULT_FLAGS | TSS_PRESENT;
        unsigned int flag = TSS_AVAILABILITY;

        global_descriptor_table[SEG_TSS].limit_1 = (uint16_t) limit;
        global_descriptor_table[SEG_TSS].base_1 = (uint16_t) base;
        global_descriptor_table[SEG_TSS].base_2 = (uint8_t)(base>>16);
        global_descriptor_table[SEG_TSS].type = type;
        global_descriptor_table[SEG_TSS].flags_limit_2 =
                (uint_8)(limit >> 16) | flag;
        global_descriptor_table[SEG_TSS].base_3 = (base >> 24);

        /* Switch to the user's page directory (stack access) */
        vm_enable_paging(p->pgdir);
        struct task_segment* ts = (struct task_segment*)p->tss;
        ts->SS0 = SEG_KERNEL_DATA << 3;
        ts->ESP0 = (uint)p->k_stack;
        ts->esp = p->tf->esp;
        ts->ss = SEG_USER_DATA << 3;
        ltr(SEG_TSS << 3);

        if(p->state == PROC_READY)
        {
                p->state = PROC_RUNNING;
                x86_drop_priv(&k_context, p->tf->esp, (void*)p->entry_point);

                /* When we get back here, the process is done running for now. */
                return;
        }
        if(p->state != PROC_RUNNABLE)
                panic("Tried to schedule non-runnable process.");

        p->state = PROC_RUNNING;

        /* Go from kernel context to process context */
        x86_context_switch(&k_context, p->context);
}

void context_restore(uintptr_t* old_context, uintptr_t new_context)
{
	x86_context_switch(old_context, new_context);
}
