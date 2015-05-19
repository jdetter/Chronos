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
#define ASM_GDT_ENTRY_GROW_DN (0x1 << 2)
#define ASM_GDT_ENTRY_WRITE   (0x1 << 1)
#define ASM_GDT_ENTRY_READ    (0x0)

/* Flags settings */
#define ASM_GDT_ENTRY_PG_GRAN (0x1 << 0x7)
#define ASM_GDT_ENTRY_SIZE    (0x1 << 0x6)

#define ASM_GDT_NULL_ENTRY \
                .word 0; \
                .word 0; \
                .word 0; \
                .word 0;


#define ASM_GDT_ENTRY(base, limit, write, executable, down_conform) \
                .word (limit) & 0xFFFF; \
                .word (base)  & 0xFFFF; \
                .byte ((base) >> 16) & 0xFF; \
                .byte ASM_GDT_ENTRY_PRESENT | ASM_GDT_ENTRY_RESERVE | ((executable) << 3) | ((write) << 1) | ((down_conform) << 2); \
                .byte (((limit) >> 28) & 0xF) | ASM_GDT_ENTRY_PG_GRAN | ASM_GDT_ENTRY_SIZE; \
                .byte ((base) >> 24) & 0x0F;

#endif
