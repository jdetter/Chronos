#ifndef _ASM_H_
#define _ASM_H_

#define SEG_PRES	((0x01 << 7) | (0x01 << 4))
#define SEG_EXE 	(0x01 << 3)
#define SEG_DATA 	0x00

#define SEG_KERN 	0x00
#define SEG_USER 	(0x03 << 5)

#define SEG_GRWU	0x00
#define SEG_GRWD 	(0x01 << 2)

#define SEG_CONF 	0x00
#define SEG_NONC 	0x01

#define SEG_READ 	0x02
#define SEG_WRITE 	0x02

#define SEG_DEFAULT_ACCESS (SEG_PRES | SEG_CONF)
#define SEG_DEFAULT_FLAGS (0xC0)
/**
 * Page granularity and 32bit entries are enabled by default.
 */

#define SEG_NULLASM \
	.word 0; \
	.word 0; \
	.byte 0; \
	.byte 0; \
	.byte 0; \
	.byte 0

#define SEG_ASM(exe_data, read_write, priv, base, limit) 	\
	.word ((limit >> 12) & 0xFFFF);				\
	.word (base & 0xFFFF);					\
	.byte ((base >> 16) & 0xFF);				\
	.byte ((SEG_DEFAULT_ACCESS | exe_data | read_write | priv) & 0xFF); \
	.byte (((limit >> 28) | SEG_DEFAULT_FLAGS) & 0xFF); 	\
	.byte ((base >> 24) & 0xFF)

#endif
