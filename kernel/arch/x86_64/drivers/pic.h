#ifndef _PIC_H_
#define _PIC_H_

/**
 * Initilize the master and slave PICs and set their offsets.
 */
void pic_init(void);

/**
 * Disable the master and slave PICs. This is needed in order to use APIC.
 */
void pic_disable(void);

/**
 * Returns the current mask of the slave pic.
 */
uint pic_slave_mask(void);

/**
 * Returns the current mask of the master pic.
 */
uint pic_master_mask(void);

/**
 * Sets the interrupt mask of the slave.
 */
void set_pic_slave_mask(uint mask);

/**
 * Sets the interrupt mask of the master.
 */
void set_pic_master_mask(uint mask);

/**
 * Let the device with the interrupt number interrupt raise interrupts.
 */
void pic_enable(uint interrupt);

/**
 * We are done handling the current interrupt and we are ready for the
 * next interrupt.
 */
void pic_eoi(uint interrupt);

/**
 * Put the PICs into the original configuration. This is needed to enter
 * real mode again.
 */
void picreset(void);

#endif
