#ifndef _DEVMAN_H_
#define _DEVMAN_H_

#include "fsman.h"

#define MAX_DEVICES 128

/* Video modes */
#define VIDEO_MODE_NONE 0x00
#define VIDEO_MODE_COLOR 0x20
#define VIDEO_MODE_MONO 0x30

/* ioctl helper function */
int ioctl_arg_ok(void* arg, size_t sz);

/* Definitions for drivers */
struct IODriver
{
	void* context; /* Pointer to driver specific context */
	slock_t device_lock;
	/* Basic io functions */
	int (*init)(struct IODriver* driver);
	/**
	 * Read sz bytes into the buffer dst. Returns the amount of
	 * bytes read.
	 */
	int (*read)(void* dst, fileoff_t start_read, size_t sz, void* context);
	
	/**
	 * Write sz bytes to the device from buffer src. Returns the amount
	 * of bytes written to the device.
	 */
        int (*write)(void* src, fileoff_t start_write, size_t sz, void* context);
	int (*ioctl)(unsigned long request, void* arg, void* context);

	/**
	 * Optional methods for checking if the IO will block.
	 */
	int (*ready_read)(void* context);
	int (*ready_write)(void* context);
};

struct DeviceDriver
{
	struct IODriver io_driver; /* general io driver, see above */
	int valid; /* Whether or not this driver is in use  */
	int type; /* The type of device, defined in include/device.h */
	char node[FILE_MAX_PATH]; /* where is the node for this driver? */
};

/**
 * Some super basic devices
 */
extern struct DeviceDriver* dev_null; /* null device driver */
extern struct DeviceDriver* dev_zero; /* zero device driver */

/**
 * Setup all io drivers for all available devices.
 */
int dev_init();

/**
 * Read from the device. Returns the amount of bytes read.
 */
int dev_read(struct IODriver* device, void* dst, 
		fileoff_t start_read, size_t sz);

/**
 * Write to an io device. Returns the amount of bytes written.
 */
int dev_write(struct IODriver* device, void* src, 
		fileoff_t start_write, size_t sz);

/**
 * Populates the dev folder with devices.
 */
void dev_populate(void);

/**
 * Lookup the driver for a device.
 */
struct DeviceDriver* dev_lookup(int dev);

#endif
