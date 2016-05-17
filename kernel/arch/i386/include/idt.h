#ifndef _IDT_H_
#define _IDT_H_

void trap_init(void);

#define GATE_USER 	(0x3 << 13)
#define GATE_KERNEL 	(0x00)
#define GATE_TASK_CONST	(0x8500)

#define MKTASK_GATE(tss, opt) {0, ((uint_16)tss), (opt | GATE_TASK_CONST), 0}

struct task_gate
{
	uint16_t reserved_1;
	uint16_t tss_segment_selector;
	uint16_t type;
	uint16_t reserved_2;
};

#define GATE_INT_CONST (0x8E00)
#define MKINT_GATE(ss, offset, opt) \
	{((uint_16)offset), (ss), ((opt) | GATE_INT_CONST), \
	((uint_16)(offset >> 8))}

struct int_gate
{
	uint16_t offset_1;
	uint16_t segment_selector;
	uint16_t flags;
	uint16_t offset_2;
};

#define GATE_TRAP_CONST (0x8F00)
#define MKTRAP_GATE(ss, offset, opt) \
        {((uint_16)offset), (ss), ((opt) | GATE_TRAP_CONST), \
        ((uint_16)(offset >> 8))}

struct trap_gate
{
        uint16_t offset_1;
        uint16_t segment_selector;
        uint16_t flags;
        uint16_t offset_2;
};

#endif
