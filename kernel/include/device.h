#ifndef _DEVICE_H_
#define _DEVICE_H_

/**
 * Definitions for device types.
 */

/* 0 is reserved for null */
#define DEV_IO 	    	0x0001 /* Basic io device. */
#define DEV_DISK        0x0002 /* Raw disk */
#define DEV_DISK_PART   0x0003 /* Disk partition */
#define DEV_TTY         0x0004 /* Teletype */
#define DEV_PIPE        0x0005 /* Pipe */
#define DEV_RAM         0x0006 /* Memory */
#define DEV_LOOP        0x0007 /* Loop device */
#define DEV_COM         0x0008 /* Serial port */
#define DEV_SOCK        0x0009 /* Socket */

#endif
