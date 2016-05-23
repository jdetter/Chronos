/**
 * Author: John Detter <john@detter.com>
 *
 * Driver for the 80*86 Programmable Interrupt Controller.
 */

#include "x86.h"
#include "drivers/pic.h"
#include "panic.h"

#define PORT_PIC_MASTER_COMMAND 	0x0020
#define PORT_PIC_MASTER_DATA 		0x0021
#define PORT_PIC_SLAVE_COMMAND		0x00A0
#define PORT_PIC_SLAVE_DATA		0x00A1

/* Command to tell the pic that we are done with this interrupt.*/
#define COMMAND_PIC_EOI			0x20
/* Command to tell the pic that it should enter setup mode. */
#define COMMAND_PIC_INIT		0x10
#define COMMAND_PIC_ICW4_NEEDED		0x01
/* Put the PIC into 8086 mode */
#define COMMAND_PIC_8086_MODE		0x01

void pic_init(void)
{
	/* 
	 * Because x86 PIC IRQs collide with the x86 hardware interrupts, we 
         * will remap them here.
	 */
	
	/* Put the PICs into init mode */
	outb(PORT_PIC_MASTER_COMMAND, 
		COMMAND_PIC_INIT | COMMAND_PIC_ICW4_NEEDED);
	io_wait();
	outb(PORT_PIC_SLAVE_COMMAND, 
		COMMAND_PIC_INIT | COMMAND_PIC_ICW4_NEEDED);
	io_wait();

	/* Send offsets */
	outb(PORT_PIC_MASTER_DATA, INT_PIC_MAP_OFF_1);
	io_wait();	
	outb(PORT_PIC_SLAVE_DATA, INT_PIC_MAP_OFF_2);	
	io_wait();

	/* Send configuration */

	/* Master has slave connected at port 2. */
        outb(PORT_PIC_MASTER_DATA, 0x04);
        io_wait();	
	/* Slave is connected to master at IRQ2 */
	outb(PORT_PIC_SLAVE_DATA, 0x02);
	io_wait();

	/* Set operation mode */
	outb(PORT_PIC_SLAVE_DATA, COMMAND_PIC_8086_MODE);
	io_wait();
	outb(PORT_PIC_MASTER_DATA, COMMAND_PIC_8086_MODE);
	io_wait();

	/* 
         * Disable all interrupts. Other drivers will need to enable their
         * Specific interrupt number. 
	 */
	set_pic_master_mask(0xFF);
	set_pic_slave_mask(0xFF);
}

void pic_disable(void)
{
	set_pic_slave_mask(0xFF);
	set_pic_master_mask(0xFF);
}

uint pic_slave_mask(void)
{
	return inb(PORT_PIC_SLAVE_DATA);
}

uint pic_master_mask(void)
{
	return inb(PORT_PIC_MASTER_DATA);
}

void set_pic_slave_mask(uint mask)
{
	outb(PORT_PIC_SLAVE_DATA, mask);
}

void set_pic_master_mask(uint mask)
{	
	outb(PORT_PIC_MASTER_DATA, mask);
}

void pic_enable(uint interrupt)
{
	interrupt -= INT_PIC_TIMER;
	uint port = PORT_PIC_MASTER_DATA;
	if(interrupt > 7)
	{
		/* We have to modify the slave. */
		interrupt -= 8;
		if(interrupt > 7) 
			panic("Invalid interrupt number: %d\n", interrupt);
		port = PORT_PIC_SLAVE_DATA;

		/* Make sure master can recieve slave interrupts */
		pic_enable(INT_PIC_SLAVE);
	}

	if(interrupt > 7) 
		panic("Invalid interrupt number: %d\n", interrupt);

	uchar curr_mask = inb(port);
	/* Create the new mask */
	curr_mask &= ~(1 << interrupt);
	/* Write the new mask. */
	outb(port, curr_mask);
}

void pic_eoi(uint interrupt)
{
	interrupt -= INT_PIC_TIMER;
	if(interrupt > 7)
	{
		/* Reset the slave */
		outb(PORT_PIC_SLAVE_COMMAND, COMMAND_PIC_EOI);
	}
	outb(PORT_PIC_MASTER_COMMAND, COMMAND_PIC_EOI);
	io_wait();
}

void picreset(void)
{
        /* Put the PICs into init mode */
        outb(PORT_PIC_MASTER_COMMAND,
                COMMAND_PIC_INIT | COMMAND_PIC_ICW4_NEEDED);
        io_wait();
        outb(PORT_PIC_SLAVE_COMMAND,
                COMMAND_PIC_INIT | COMMAND_PIC_ICW4_NEEDED);
        io_wait();

        /* Send offsets */
        outb(PORT_PIC_MASTER_DATA, INT_PIC_ORIG_OFF_1);
        io_wait();
        outb(PORT_PIC_SLAVE_DATA, INT_PIC_ORIG_OFF_2); 
        io_wait();

        /* Send configuration */

        /* Slave is connected to master at IRQ2 */
        outb(PORT_PIC_SLAVE_DATA, 0x02);
        io_wait();
        /* Master has slave connected at port 2. */
        outb(PORT_PIC_MASTER_DATA, 0x04);
        io_wait();

        /* Set operation mode */
        outb(PORT_PIC_SLAVE_DATA, COMMAND_PIC_8086_MODE);
        io_wait();
        outb(PORT_PIC_MASTER_DATA, COMMAND_PIC_8086_MODE);
        io_wait();

        /* Clear the masks. */
        set_pic_master_mask(0x0);
        set_pic_slave_mask(0x0);
}
