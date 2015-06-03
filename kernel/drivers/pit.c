#include "types.h"
#include "x86.h"
#include "pic.h"

#define TIMER_FREQ 1193182

#define PORT_PIT_CHANNEL_0_DATA 0x40
#define PORT_PIT_CHANNEL_1_DATA 0x41 /* Obsolete*/
#define PORT_PIT_CHANNEL_2_DATA 0x42
#define PORT_PIT_COMMAND 	0x43

void pit_init(void)
{
	/* The command we will send */
	uchar command = 0;
	/* Set the channel */
	command |= 0 << 6; /* Channel 0 */
	/* Access mode */
	command |= 3 << 4; /* Lo/Hi mode */
	/* Output mode */
	command |= 2 << 1; /* Rate generator */
	/* BCD / Binary */
	command |= 0; /* Input is binary, not BCD */

	/* Write the command */
	outb(PORT_PIT_COMMAND, command);

	/* How many ticks / second do you want? */
	uint ticks = 1;

	/* Calculate the correct divisor. */
	uint divisor = TIMER_FREQ / ticks;
	/* Write the divisor (low) */
	outb(PORT_PIT_CHANNEL_0_DATA, (uchar)divisor);
	/* Write the divisor (high) */
	outb(PORT_PIT_CHANNEL_0_DATA, (uchar)(divisor >> 8));
	pic_enable(INT_PIC_TIMER);
}
