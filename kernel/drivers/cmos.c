#include "types.h"
#include "stdlock.h"
#include "stdlib.h"
#include "cpu.h"
#include "x86.h"

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
}

uchar cmos_read(uchar reg)
{
	slock_acquire(&cmos_lock);
	uchar nmi_mask = 0x0;
	if(nmi_enabled) nmi_mask = 0x80;

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
	uchar old = inb(CMOS_COMMAND);
	io_wait();
	if(nmi_enabled) /* clear the 7th bit 0x7F = 01111111*/
		outb(CMOS_COMMAND, old & 0x7F);
	else /* Set the 7th bit 0x80 = 10000000*/
		outb(CMOS_COMMAND, old | 0x80);

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
