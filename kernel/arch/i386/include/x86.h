#ifndef _X86_H_
#define _X86_H_

/* Intel 64 and IA-32 Architectures Software Developer's Manual: */
/* http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-manual-325462.pdf*/

/* IA32 Traps numbers from the reference */
#define TRAP_DE 0x00 /* Divide Error (DIV and IDIV instructions) */
#define TRAP_DB 0x01 /* Debug (code or data reference) */
#define TRAP_N1 0x02 /* Non maskable interrupt */
#define TRAP_BP 0x03 /* Break received */
#define TRAP_OF 0x04 /* Overflow caught */
#define TRAP_BR 0x05 /* BOUND Range Exceeded (BOUND Instruction) */
#define TRAP_UD 0x06 /* Invlid Opcode (Invalid instruction) */
#define TRAP_NM 0x07 /* Device not available */
#define TRAP_DF 0x08 /* Double Fault (NMI or INTR) */
#define TRAP_MF 0x09 /* Coprocessor Segment Overrun (reserved) */


#define TRAP_TS 0x0A /* Invalid TSS (Invalid task switch or TSS access) */
#define TRAP_NP 0x0B /* Segment Not Present (Loading segment registers) */
#define TRAP_SS 0x0C /* Stack Segment Fault (Stack operations SS) */
#define TRAP_GP 0x0D /* General Protection (protection checks) */
#define TRAP_PF 0x0E /* Page Fault (Memory not mapped in uvm) */
#define TRAP_0F 0x0F /* RESERVED */
#define TRAP_FP 0x10 /* Floating Point Error */
#define TRAP_AC 0x11 /* Alignment Check */
#define TRAP_MC 0x12 /* Machine Check */
#define TRAP_XM 0x13 /* SIMD floating-point exception */
/* 0x14 - 0x1F are reserved */

/* Chronos Trap Definitions are in chronos.h */

/* EFLAGS definitions */
#define EFLAGS_CF	(0x01 << 0)
#define EFLAGS_PF	(0x01 << 2)
#define EFLAGS_AF	(0x01 << 4)
#define EFLAGS_ZF	(0x01 << 6)
#define EFLAGS_SF	(0x01 << 7)
#define EFLAGS_TF	(0x01 << 8)
#define EFLAGS_IF	(0x01 << 9)
#define EFLAGS_DF	(0x01 << 10)
#define EFLAGS_OF	(0x01 << 11)
#define EFLAGS_IOPL	(0x03 << 12)
#define EFLAGS_NT	(0x01 << 14)


/* CR0 bits */
#define CR0_MP          (0x01 << 1)
#define CR0_EM          (0x01 << 2)
#define CR0_WP		(0x01 << 16)
#define CR0_PGENABLE 	(0x01 << 31)

/* CR4 bits */
#define CR4_OSFXSR      (0x01 << 9)
#define CR4_OSXMMEXCPT  (0x01 << 10)

/* Segment table bits */
#define SEG_PRESENT     (0x01 << 7)
#define SEG_PRES        ((0x01 << 7) | (0x01 << 4))
#define SEG_EXE         (0x01 << 3)
#define SEG_DATA        0x00

#define SEG_KERN        0x00
#define SEG_USER        (0x03 << 5)

#define SEG_GRWU        0x00
#define SEG_GRWD        (0x01 << 1)

#define SEG_CONF        0x00
#define SEG_NONC        (0x01 << 2)

#define SEG_READ        0x02
#define SEG_WRITE       0x02

#define SEG_SZ          0x40

#define SEG_DEFAULT_ACCESS (SEG_PRES | SEG_CONF)
#define SEG_DEFAULT_FLAGS (0xC0)

/* Segment creators */
#define SEG_NULLASM \
        .word 0; \
        .word 0; \
        .byte 0; \
        .byte 0; \
        .byte 0; \
        .byte 0

#define SEG_ASM(exe_data, read_write, priv, base, limit)        \
        .word ((limit >> 12) & 0xFFFF);                         \
        .word (base & 0xFFFF);                                  \
        .byte ((base >> 16) & 0xFF);                            \
        .byte ((SEG_DEFAULT_ACCESS | exe_data | read_write | priv) & 0xFF); \
        .byte (((limit >> 28) | SEG_DEFAULT_FLAGS) & 0xFF);     \
        .byte ((base >> 24) & 0xFF)

/* definitions for relevant MSRs:*/
#define SYSENTER_CS_MSR         0x174
#define SYSENTER_ESP_MSR        0x175
#define SYSENTER_EIP_MSR        0x176

#define CONSOLE_MONO_BASE_ORIG  (0xB0000)
#define CONSOLE_COLOR_BASE_ORIG (0xB8000)

#define ATA_CACHE_SZ 0x30000 /* HDD cache sz per drive */

#ifndef __ASM_ONLY__

#include "context.h"
#include "vm.h"

/* Unsigned types */
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

/* data type sizes for i386 */
typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef unsigned long long int uint_64;

/* Inline assembly functions */
static inline uchar
inb(ushort port)
{
  uchar data;

  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}

static inline void
insl(int port, void *addr, int cnt)
{
  asm volatile("cld; rep insl" :
               "=D" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "memory", "cc");
}

static inline void
outb(ushort port, uchar data)
{
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline void
outw(ushort port, ushort data)
{
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline void
outsl(int port, const void *addr, int cnt)
{
  asm volatile("cld; rep outsl" :
               "=S" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "cc");
}

static inline void
lgdt(uint table_addr, int size)
{
	volatile ushort descriptor[3];
	descriptor[0] = size - 1;
	descriptor[1] = (uint)((uint)table_addr & 0xFFFF);
	descriptor[2] = (uint)(((uint)table_addr >> 16) & 0xFFFF);

	asm volatile("lgdt (%0)" : : "r" (descriptor));
}

static inline void
lidt(uint table_addr, int size)
{
	volatile ushort descriptor[3];
        descriptor[0] = size - 1;
        descriptor[1] = (uint)((uint)table_addr & 0xFFFF);
        descriptor[2] = (uint)(((uint)table_addr >> 16) & 0xFFFF);

        asm volatile("lidt (%0)" : : "r" (descriptor));
}

/**
 * Disable interrupts.
 */
static inline void cli(void)
{
	asm volatile("cli");
}

/**
 * Enable interrupts.
 */
static inline void sti(void)
{
	asm volatile("sti");
}

/**
 * Load the task selector register.
 */
static inline void ltr(ushort ts)
{
	asm volatile("ltr %0" : : "r" (ts));
}

/**
 * Busy work for waiting for an io to finish.
 */
static inline void io_wait(void)
{
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

/* Concurrency x86 instructions. */

/**
 * Return the value at variable and return it. This function will also
 * add inc to the current value of variable atomically.
 */
static inline int fetch_and_add(int* variable, int inc) 
{
	asm volatile("lock; xaddl %%eax, %2;"
			:"=a" (inc)
			:"a" (inc), "m" (*variable)
			:"memory");
	return inc;
}

/**
 * Exchange the value in addr for newval and return the original value of addr.
 * This happens atomically.
 */
static inline int xchg(volatile int *addr, int newval)
{
	int result;
	asm volatile("lock; xchgl %0, %1" :
			"+m" (*addr), "=a" (result) :
			"1" (newval) :
			"cc");
	return result;
}

/**
 * Returns the value in the 0th control register.
 */
extern unsigned int x86_get_cr0(void);

/**
 * Returns the value in the 2th control register.
 */
extern unsigned int x86_get_cr2(void);

/**
 * Returns the value in the 3th control register.
 */
extern unsigned int x86_get_cr3(void);

/**
 * Returns the value in the 4th control register.
 */
extern unsigned int x86_get_cr4(void);

/**
 * Check to see if interrupts are enabled.
 */
extern int x86_check_interrupt(void);

/**
 * Move from the current context to a new context.
 */
extern void x86_context_switch(context_t* current, context_t next);

/**
 * Lower privileges and enter a new user function.
 */
extern void x86_drop_priv(context_t* context, pstack_t new_stack, void* entry);

/**
 * Reboot the system
 */
extern void x86_reboot(void);

/**
 * Shut down the system
 */
extern void x86_shutdown(void);

#endif

/**
 * We are remapping the original PIC definitions. The new defs are here.
 */

#define INT_PIC_MAP_OFF_1 0x20
#define INT_PIC_MAP_OFF_2 0x28

#define INT_PIC_ORIG_OFF_1 0x08
#define INT_PIC_ORIG_OFF_2 0x70

#define INT_PIC_TIMER 		0x00 + INT_PIC_MAP_OFF_1
#define INT_PIC_KEYBOARD 	0x01 + INT_PIC_MAP_OFF_1
#define INT_PIC_SLAVE 		0x02 + INT_PIC_MAP_OFF_1
#define INT_PIC_COM2 		0x03 + INT_PIC_MAP_OFF_1
#define INT_PIC_COM1 		0x04 + INT_PIC_MAP_OFF_1
#define INT_PIC_LPT2 		0x05 + INT_PIC_MAP_OFF_1
#define INT_PIC_FLOPPY 		0x06  + INT_PIC_MAP_OFF_1
#define INT_PIC_LPT1 		0x07  + INT_PIC_MAP_OFF_1
#define INT_PIC_CMOS 		0x00  + INT_PIC_MAP_OFF_2
#define INT_PIC_INT09 		0x01  + INT_PIC_MAP_OFF_2
#define INT_PIC_INT10	 	0x02  + INT_PIC_MAP_OFF_2
#define INT_PIC_INT11 		0x03  + INT_PIC_MAP_OFF_2
#define INT_PIC_MOUSE 		0x04  + INT_PIC_MAP_OFF_2
#define INT_PIC_COPROCESSOR 	0x05  + INT_PIC_MAP_OFF_2
#define INT_PIC_ATA1 		0x06  + INT_PIC_MAP_OFF_2
#define INT_PIC_ATA2 		0x07  + INT_PIC_MAP_OFF_2

#define INT_PIC_TIMER_CODE      0x00
#define INT_PIC_KEYBOARD_CODE	0x01
#define INT_PIC_SLAVE_CODE      0x02
#define INT_PIC_COM2_CODE       0x03
#define INT_PIC_COM1_CODE       0x04
#define INT_PIC_LPT2_CODE       0x05
#define INT_PIC_FLOPPY_CODE     0x06
#define INT_PIC_LPT1_CODE       0x07
#define INT_PIC_CMOS_CODE       0x08
#define INT_PIC_INT09_CODE      0x09
#define INT_PIC_INT10_CODE      0x0A
#define INT_PIC_INT11_CODE      0x0B
#define INT_PIC_MOUSE_CODE      0x0C
#define INT_PIC_COPROCESSOR_CODE 0x0D
#define INT_PIC_ATA1_CODE       0x0E
#define INT_PIC_ATA2_CODE       0x0F

#ifndef __ASM_ONLY__
struct task_segment
{
	uint16_t previous_task_link; /* Segment selector of prev task*/
	uint16_t reserved_1;
	uint32_t ESP0;
	uint16_t SS0;
	uint16_t reserved_2;
	uint32_t ESP1;
	uint16_t SS1;
	uint16_t reserved_3;
	uint32_t ESP2;
	uint16_t SS2;
	uint16_t reserved_4;
	uint32_t cr3; /* Saved page table base register */
	uint32_t eip; /* Instruction pointer */
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint16_t es;
	uint16_t reserved5;
	uint16_t cs;
	uint16_t reserved6;
	uint16_t ss;
	uint16_t reserved7;
	uint16_t ds;
	uint16_t reserved8;
	uint16_t fs;
	uint16_t reserved9;
	uint16_t gs;
	uint16_t reserved10;
	uint16_t ldt;
	uint16_t reserved11;
	uint16_t T;
	uint16_t io_base_address_map;
}; /* Size MUST be 104.*/
#endif

#endif
