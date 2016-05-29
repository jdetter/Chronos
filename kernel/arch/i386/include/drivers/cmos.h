#ifndef _CMOS_H_
#define _CMOS_H_

#include "x86.h"

/**
 * Init the cmos chip. This will disable NMI by default. 
 */
void cmos_init(void);

/**
 * Read a CMOS register. Returns the value in the CMOS register.
 */
uchar cmos_read(uchar reg);

/**
 * Enable Non-maskable Interrupts.
 */
void nmi_enable(void);

/**
 * Disable Non-maskable Interrupts.
 */
void nmi_disable(void);

/**
 * Read the CMOS interrupt register.
 */
uchar cmos_read_interrupt(void);

#endif
