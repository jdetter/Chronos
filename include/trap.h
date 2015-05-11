#ifndef TRAP_H_

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
#define TRAP_MF 0x10 /* Floating Point Error */
#define TRAP_AC 0x11 /* Alignment Check */
#define TRAP_MC 0x12 /* Machine Check */
#define TRAP_XM 0x13 /* SIMD floating-point exception */
/* 0x14 - 0x1F are reserved */

/* Chronos Trap Definitions */
#define TRAP_SC 0x20 /* System call trap*/

#endif
