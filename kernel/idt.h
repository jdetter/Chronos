#ifndef _IDT_H_
#define _IDT_H_

void trap_init(void);

#define GATE_USER 	(0x3 << 13)
#define GATE_KERNEL 	(0x00)
#define GATE_TASK_CONST	(0x8500)

#define MKTASK_GATE(tss, opt) {0, ((uint_16)tss), (opt | GATE_TASK_CONST), 0}

struct task_gate
{
	uint_16 reserved_1;
	uint_16 tss_segment_selector;
	uint_16 type;
	uint_16 reserved_2;
};

#define GATE_INT_CONST (0x8E00)
#define MKINT_GATE(ss, offset, opt) \
	{((uint_16)offset), (ss), ((opt) | GATE_INT_CONST), \
	((uint_16)(offset >> 8))}

struct int_gate
{
	uint_16 offset_1;
	uint_16 segment_selector;
	uint_16 flags;
	uint_16 offset_2;
};

#define GATE_TRAP_CONST (0x8F00)
#define MKTRAP_GATE(ss, offset, opt) \
        {((uint_16)offset), (ss), ((opt) | GATE_TRAP_CONST), \
        ((uint_16)(offset >> 8))}

struct trap_gate
{
        uint_16 offset_1;
        uint_16 segment_selector;
        uint_16 flags;
        uint_16 offset_2;
};

#endif
