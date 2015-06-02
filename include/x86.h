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

/* Chronos Trap Definitions are in chronos.h */

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

#define PGDIR_PRESENT	(1 << 0x1)
#define PGDIR_RW 	(1 << 0x2)
#define PGDIR_USER	(1 << 0x4)
#define PGDIR_WTWB 	(1 << 0x8)
#define PGDIR_DCACHE 	(1 << 0x10)
#define PGDIR_ACCESSED	(1 << 0x20)
/* Reserved */
#define PGDIR_SIZE	(1 << 0x80)
#define PGDIR_GLBL	(1 << 0x100)

#define PGTBL_PRESENT   (1 << 0x1)
#define PGTBL_RW        (1 << 0x2)
#define PGTBL_USER      (1 << 0x4)
#define PGTBL_WTWB      (1 << 0x8)
#define PGTBL_CACHE_DIS (1 << 0x10)
#define PGTBL_ACCESSED  (1 << 0x20)
#define PGTBL_DIRTY     (1 << 0x40)
/* Reserved */

#define PGSIZE 4096

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
static inline uint xchg(volatile uint *addr, uint newval)
{
	uint result;
	asm volatile("lock; xchgl %0, %1" :
			"+m" (*addr), "=a" (result) :
			"1" (newval) :
			"cc");
	return result;
}

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

struct task_segment
{
	uint_16 previous_task_link; /* Segment selector of prev task*/
	uint_16 reserved_1;
	uint_32 ESP0;
	uint_16 SS0;
	uint_16 reserved_2;
	uint_32 ESP1;
	uint_16 SS1;
	uint_16 reserved_3;
	uint_32 ESP2;
	uint_16 SS2;
	uint_16 reserved_4;
	uint_32 cr3; /* Saved page table base register */
	uint_32 eip; /* Instruction pointer */
	uint_32 eflags;
	uint_32 eax;
	uint_32 ecx;
	uint_32 edx;
	uint_32 ebx;
	uint_32 esp;
	uint_32 ebp;
	uint_32 esi;
	uint_32 edi;
	uint_16 es;
	uint_16 reserved5;
	uint_16 cs;
	uint_16 reserved6;
	uint_16 ss;
	uint_16 reserved7;
	uint_16 ds;
	uint_16 reserved8;
	uint_16 fs;
	uint_16 reserved9;
	uint_16 gs;
	uint_16 reserved10;
	uint_16 ldt;
	uint_16 reserved11;
	uint_16 reserved12;
	uint_16 io_base_address_map;
}; /* Size MUST be 104.*/

#endif
