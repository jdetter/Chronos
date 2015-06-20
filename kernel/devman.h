#ifndef _DEVMAN_H_
#define _DEVMAN_H_

#define MAX_DEVICES 128

/* Video modes */
#define VIDEO_MODE_NONE 0x00
#define VIDEO_MODE_COLOR 0x20
#define VIDEO_MODE_MONO 0x30

/* Definitions for drivers */

#define IO_DRIVER_CONTEXT_SPACE 128
struct IODriver
{
	uchar valid; /* Whether or not this driver is in use. */
	slock_t device_lock;
	int (*init)(struct IODriver* driver);
	int (*read)(void* dst, uint start_read, uint sz, void* context);
        int (*write)(void* src, uint start_write, uint sz, void* context);
	void* context;
	char node[FILE_MAX_PATH]; /* where is the node for this driver? */
};

extern struct IODriver io_drivers[];

/**
 * Setup all io drivers for all available devices.
 */
int dev_init();

/**
 * Read from the device. Returns the amount of bytes read.
 */
int dev_read(struct IODriver* device, void* dst, uint start_read, uint sz);

/**
 * Write to an io device. Returns the amount of bytes written.
 */
int dev_write(struct IODriver* device, void* src, uint start_write, uint sz);

/**
 * Populates the dev folder with devices.
 */
void dev_populate(void);

/**
 * Lookup the driver for a device.
 */
struct IODriver* dev_lookup(int dev_type, int dev);

#endif
