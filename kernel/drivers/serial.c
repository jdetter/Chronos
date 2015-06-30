#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
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

uchar serial_started = 0;
uchar serial_connected = 0;
uint serial_received(void);
uint transmit_ready(void);

int serial_init(int pic)
{
	if(serial_started) return !serial_connected;
	serial_started = 1;
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

	/* Is there a serial connection? */
	if(inb(COM1_LSR) == 0xFF)
	{
		serial_connected = 0;
		return 1;
	}

	/* Clear pending interrupts */
	inb(COM1_INT_IDENT);
	inb(COM1_DATA);

	/* Don't enable pic here anymore */

	serial_connected = 1;
	return 0;
}

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

uint serial_write(void* src, uint sz)
{
	int x = 0;
	uchar* src_c = (uchar*)src;
	for(;sz;sz--)
	{
		/* Wait for the controller to be ready. */
		while(!transmit_ready());
		/* Write a byte. */
		outb(COM1_DATA, *src_c);
		/* Move the pointer forward. */
		src_c++;
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

char serial_read_noblock(void)
{
	if(serial_received())
		return inb(COM1_DATA);

	return 0;
}

int serial_io_init(struct IODriver* driver);
int serial_io_read(void* dst, uint start_read, uint sz, void* context);
int serial_io_write(void* src, uint start_write, uint sz, void* context);
int serial_io_setup(struct IODriver* driver)
{
	driver->init = serial_io_init;
	driver->read = serial_io_read;
	driver->write = serial_io_write;
	return 0;
}

int serial_io_init(struct IODriver* driver)
{
	return 0;
}

int serial_io_read(void* dst, uint start_read, uint sz, void* context)
{
	return serial_read(dst, sz);
}

int serial_io_write(void* src, uint start_write, uint sz, void* context)
{
	return serial_write(src, sz);
}
