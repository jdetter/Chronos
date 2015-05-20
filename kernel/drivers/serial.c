#include "types.h"
#include "serial.h"
#include "x86.h"
#include "pic.h"

#define COM1_DATA 		0x03F8
#define COM1_INT  		0x03F9
#define COM1_INT_IDENT 		0x03FA
#define COM1_LCR 		0x03FB
#define COM1_MCR 		0x03FC
#define COM1_LSR 		0x03FD
#define COM1_MSR 		0x03FE
#define COM1_SCRATCH 		0x03FF

#define COM1_IRQ 0x04

uint serial_received(void);
uint transmit_ready(void);

void serial_init(int pic)
{
	/* Turn off FIFO */
	outb(COM1_INT_IDENT, 0);

	/* Enable frequency divisor */
	outb(COM1_LCR, 0x80);
	/* Set frequency */
	outb(COM1_DATA, 12); /* 9600 = 115200 / 12 */
	outb(COM1_INT, 0); /* hi byte for frequency */
	/* 8 bits, no parity, one stop bit. */
	outb(COM1_LCR, 0x03);
	outb(COM1_MCR, 0x00); 
	outb(COM1_INT, 0x01); /* Allow interrupts */

	if(inb(COM1_LSR) == 0xFF)
		for(;;); /* PANIC */

	/* Clear pending interrupts */
	inb(COM1_INT_IDENT);
	inb(COM1_DATA);

	/* Should we enable pic? */
	if(pic) pic_enable(COM1_IRQ);
}

/**
 * Is there something waiting to be read?
 */
uint serial_received(void)
{
	return inb(COM1_LSR) & 0x1;
}

/**
 * Is the buffer ready to be written to?
 */
uint transmit_ready(void)
{
	return inb(COM1_LSR) & 0x20;
}

uint serial_write(void* dst, uint sz)
{
	int x = 0;
	uchar* dst_c = (uchar*)dst;
	for(;sz;sz--)
	{
		/* Wait for the controller to be ready. */
		while(!transmit_ready());
		/* Write a byte. */
		outb(COM1_DATA, *dst_c);
		/* Move the pointer forward. */
		dst_c++;
		x++;
	}

	return x;
}

uint serial_read(void* dst, uint sz)
{
	int x = 0;
	uchar* dst_c = (uchar*)dst;
	for(;sz;sz--)
	{
		/* Wait for the controller to be ready. */
		while(!serial_received());
		/* Read a byte. */
		uchar b = inb(COM1_DATA);
		/* Put byte into buffer. */
		*dst_c = b;
		/* Move the pointer forward. */
		dst_c++;
		x++;
	}

	return x;
}
