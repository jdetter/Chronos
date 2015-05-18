#ifndef _X86_H_
#define _X86_H_

/* Intel 64 and IA-32 Architectures Software Developer's Manual: */
/* http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-manual-325462.pdf*/

/* IA32 Traps numbers from the reference */
#define TRAP_DE 0x00 /* Divide Error (DIV and IDIV instructions) */
#define TRAP_DB 0x01 /* Debug (code or data reference) */
#define TRAP_BP 0x02 /* Break received */
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

/* Chronos Trap Definitions */
#define TRAP_SC 0x20 /* System call trap*/

/* Intel page table and page directory flags */
#define PG_PRESENT(pg) (pg & 0x1)
#define PG_RW(pg) (pg & 0x2)
#define PG_USER(pg) (pg & 0x4)
#define PG_WTWB(pg) (pg & 0x8)
#define PG_DCACHE(pg) (pg & 0x10)
#define PG_ACCESSED(pg) (pg & 0x20)
/* Reserved */
#define PG_SIZE(pg) (pg & 0x80)
#define PG_GLBL(pg) (pg & 0x100)

#define PG_FLG9(pg) (pg & 0x200)
#define PG_FLG10(pg) (pg & 0x400)
#define PG_FLG11(pg) (pg & 0x800)

#define PG_PRESENT_SET(pg) (pg | 0x1)
#define PG_RW_SET(pg) (pg | 0x2)
#define PG_USER_SET(pg) (pg | 0x4)
#define PG_WTWB_SET(pg) (pg | 0x8)
#define PG_DCACHE_SET(pg) (pg | 0x10)
#define PG_ACCESSED_SET(pg) (pg | 0x20)
/* Reserved */
#define PG_SIZE_SET(pg) (pg | 0x80)
#define PG_GLBL_SET(pg) (pg | 0x100)

#define PG_FLG9_SET(pg) (pg | 0x200)
#define PG_FLG10_SET(pg) (pg | 0x400)
#define PG_FLG11_SET(pg) (pg | 0x800)

#define PGSIZE 4096

/* Inline assmebly functions */

/**
 * Read a byte from a port.
 */
static inline uchar inb(ushort port)
{
	uchar data;

	asm volatile("in %1,%0" : "=a" (data) : "d" (port));
	return data;
}

/**
 * Write the byte data to a port.
 */
static inline void outb(ushort port, uchar data)
{
	asm volatile("out %0,%1" : : "a" (data), "d" (port));
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

/* Concurrency x86 instructions. */

/**
 * Return the value at variable and return it. This function will also
 * add inc to the current value of variable atomically.
 */
inline int fetch_and_add(int* variable, int inc) 
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
static inline uint xchg(volatile uint *addr, uint newval)
{
	uint result;
	asm volatile("lock; xchgl %0, %1" :
			"+m" (*addr), "=a" (result) :
			"1" (newval) :
			"cc");
	return result;
}



#endif
