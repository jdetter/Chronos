/**
 * Author: John Detter <john@detter.com>
 *
 * Driver for the 80*86 Programmable Interval Timer.
 */

#include "x86.h"
#include "drivers/pic.h"

#define TIMER_FREQ 1193182

#define PORT_PIT_CHANNEL_0_DATA 0x40
#define PORT_PIT_CHANNEL_1_DATA 0x41 /* Obsolete*/
#define PORT_PIT_CHANNEL_2_DATA 0x42
#define PORT_PIT_COMMAND 	0x43

#define TICKS_PER_SECOND 1
// FAST i
#define TICKS_DIVISOR (8192)
// SLOWEST
// #define TICKS_DIVISOR 6553

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

	/* Calculate the correct divisor. */
	uint divisor = TICKS_DIVISOR;
	/* Write the divisor (low) */
	outb(PORT_PIT_CHANNEL_0_DATA, (uchar)divisor);
	/* Write the divisor (high) */
	outb(PORT_PIT_CHANNEL_0_DATA, (uchar)(divisor >> 8));
	pic_enable(INT_PIC_TIMER);
}

void pit_reset(void)
{
	/* Calculate the correct divisor. */
	uint divisor = TICKS_DIVISOR;
	/* Write the divisor (low) */
	outb(PORT_PIT_CHANNEL_0_DATA, (uchar)divisor);
	/* Write the divisor (high) */
	outb(PORT_PIT_CHANNEL_0_DATA, (uchar)(divisor >> 8));
	pic_enable(INT_PIC_TIMER);
}
