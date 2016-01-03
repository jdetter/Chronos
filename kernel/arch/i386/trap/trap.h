#ifndef _ARCH_TRAP_H_
#define _ARCH_TRAP_H_

/* Segment descriptions */
/* Null segment         0x00 */
#define SEG_KERNEL_CODE 0x01
#define SEG_KERNEL_DATA 0x02
#define SEG_USER_CODE   0x03 /* data must be 1 away from code (sysexit) */
#define SEG_USER_DATA   0x04
#define SEG_TSS         0x05
#define SEG_COUNT       0x06

#define SYS_EXIT_BASE   ((SEG_USER_CODE << 3) - 16)

#ifndef __ASM_ONLY__

#define TF_REGISTERS (0x08 << 2)
struct trap_frame
{
        /* 32 bit Registers from pushad */
        uint_32 edi;
        uint_32 esi;
        uint_32 ebp;
        uint_32 espx; /* This gets restored, but overridden by esp below */
        uint_32 ebx;
        uint_32 edx;
        uint_32 ecx;
        uint_32 eax;

        /* Segment registers */
        uint_16 gs;
        uint_16 padding1;
        uint_16 fs;
        uint_16 padding2;
        uint_16 es;
        uint_16 padding3;
        uint_16 ds;
        uint_16 padding4;

        /* Trap number pushed by trap handlers */
        uint_32 trap_number;

        /* Pushed during the interrupt */
        uint_32 error;
        uint_32 eip; /* Instruction Pointer */
        uint_16 cs; /* Code segment selector */
        uint_16 padding5; /* 16 bit padding */
        uint_32 eflags; /* Status bits for this process. */

        /* Pushed if there was a privilege change */
        uint_32 esp; /* The stack pointer that will be restored. (see espx) */
        uint_16 ss; /* Stack segment selector. */
        uint_16 padding6;
};

struct context
{
        uint_32 cr0; /* Saved page table base register */

        uint_32 edi; /* pushal */
        uint_32 esi;
        uint_32 ebp;
        uint_32 esp;
        uint_32 ebx;
        uint_32 edx;
        uint_32 ecx;
        uint_32 eax;

        uint_32 eip;
};

#endif

#endif
