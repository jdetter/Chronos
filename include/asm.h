#ifndef _ASM_H_
#define _ASM_H_

/* Assembly declarations of global descriptor table settings */
/* Access byte settings*/
#define ASM_GDT_ENTRY_PRESENT (0x1 << 0x7)
#define ASM_GDT_ENTRY_KERNEL  (0x0)
#define ASM_GDT_ENTRY_USER    (0x3 << 5)
#define ASM_GDT_ENTRY_RESERVE (0x1 << 4)
#define ASM_GDT_ENTRY_CODE    (0x1 << 3)
#define ASM_GDT_ENTRY_GROW_UP (0x0)

#define STA_X     0x8       // Executable segment
#define STA_E     0x4       // Expand down (non-executable segments)
#define STA_C     0x4       // Conforming code segment (executable only)
#define STA_W     0x2       // Writeable (non-executable segments)
#define STA_R     0x2       // Readable (executable segments)
#define STA_A     0x1       // Accessed

#define SEG_NULLASM                                             \
        .word 0, 0;                                             \
        .byte 0, 0, 0, 0

// The 0xC0 means the limit is in 4096-byte units
// and (for executable segments) 32-bit mode.
#define SEG_ASM(type,base,lim)                                  \
        .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);      \
        .byte (((base) >> 16) & 0xff), (0x90 | (type)),         \
                (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)



#endif
