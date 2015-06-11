#ifndef _DEVMAN_H_
#define _DEVMAN_H_

/* Definitions for drivers */

#define IO_DRIVER_CONTEXT_SPACE 2048
struct IODriver
{
	uchar valid; /* Whether or not this driver is in use. */
	int (*read)(void* dst, uint start_read, uint sz,
                struct FSHardwareDriver* driver);
        int (*write)(void* src, uint start_write, uint sz,
                struct FSHardwareDriver* driver);
	char context_space[IO_DRIVER_CONTEXT_SPACE];
};

#endif
