#include "types.h"
#include "idt.h"
#include "trap.h"
#include "chronos.h"
#include "x86.h"
#include "panic.h"
#include "pic.h"
#include "serial.h"

#define TRAP_COUNT 256
#define INTERRUPT_TABLE_SIZE (sizeof(struct int_gate) * TRAP_COUNT)

struct int_gate interrupt_table[TRAP_COUNT];
extern uint trap_handlers[];

void trap_init(void)
{
	int x;
	for(x = 0;x < TRAP_COUNT;x++)
	{
		interrupt_table[x].offset_1 = (uint)(trap_handlers[x] & 0xFFFF);
		interrupt_table[x].offset_2 = 
			(trap_handlers[x] >> 16) & 0xFFFF;
		interrupt_table[x].segment_selector = SEG_KERNEL_CODE << 3;
		interrupt_table[x].flags = GATE_INT_CONST;
	}

	lidt((uint)interrupt_table, INTERRUPT_TABLE_SIZE);		
}

void trap_handler(struct trap_frame* tf)
{
	int trap = tf->trap_number;
	
	if(trap == 0x20)
	{
		serial_write("timer\n", 6);
	} else serial_write("Other\n", 6);

	pic_eoi(trap);
	for(;;);
}
