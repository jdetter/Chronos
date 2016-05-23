#include <string.h>
#include <stdlib.h>

#include "stdlock.h"
#include "cpu.h"
#include "x86.h"
#include "drivers/pic.h"
#include "panic.h"
#include "drivers/cmos.h"

#define INT8_ENABLE_INTERVAL	0x40
#define INT8_ENABLE_ALARM	0x20
#define INT8_ENABLE_UPDATE	0x10

#define CMOS_COMMAND 	0x70
#define CMOS_DATA	0x71

static uchar nmi_enabled; /* Are non maskable interrupts enabled? */
static void nmi_update(void);

/* Lock needed to touch the cmos */
static slock_t cmos_lock;

void cmos_init(void)
{
	nmi_enabled = 0;
	nmi_update();
	slock_init(&cmos_lock);
	
	/* Enable the CMOS interrupt in the pic */
	pic_enable(INT_PIC_CMOS);

	/* Change frequency */
	//uint rate = 0x0D;
	//outb(CMOS_COMMAND, 0x8A); /* Select register A*/
	//uchar regA = inb(CMOS_DATA);
	//outb(CMOS_COMMAND, 0x8A);
	//outb(CMOS_COMMAND, (regA & 0xF0) | (rate & 0x0F));

	/* Change seconds alarm */
	//outb(CMOS_COMMAND, 0x81); /* Select register 0x01 */
	//outb(CMOS_DATA, 0x05);

	/* Change minutes alarm */
	//outb(CMOS_COMMAND, 0x83); /* Select register 0x03*/
	//outb(CMOS_DATA, 0x00);

	/* Enable interrupt 8 - alarm */
	outb(CMOS_COMMAND, 0x8B); /* Select register B*/
	uchar regB = inb(CMOS_DATA) | INT8_ENABLE_UPDATE;
	outb(CMOS_COMMAND, 0x8B);
	outb(CMOS_DATA, regB); /* Write int8_enable flag */

	nmi_update();
}

uchar cmos_read(uchar reg)
{
	slock_acquire(&cmos_lock);
	uchar nmi_mask = 0x0;
	if(!nmi_enabled) nmi_mask = 0x80;

	/* Or the mask with the register */
	uchar mask = nmi_mask | reg;
	/* Select the register */
	outb(CMOS_COMMAND, mask);
	/* waste some time */
	io_wait();
	/* return the value */
	uchar val = inb(CMOS_DATA);

	slock_release(&cmos_lock);
	return val;
}

static void nmi_update(void)
{
	slock_acquire(&cmos_lock);
	io_wait();
	if(nmi_enabled) 
		outb(CMOS_COMMAND, 0x0A);
	else 
		outb(CMOS_COMMAND, 0x8A);

	/* Read a value to reset the cmos register */
	inb(CMOS_COMMAND);
	slock_release(&cmos_lock);
}

void nmi_enable(void)
{
	if(nmi_enabled) return;
	nmi_enabled = 1;
	nmi_update();
}

void nmi_disable(void)
{
	if(!nmi_enabled) return;
	nmi_enabled = 0;
	nmi_update();
}

uchar cmos_read_interrupt(void)
{
	return cmos_read(0x0C);
}
