#ifndef _DEVMAN_H_
#define _DEVMAN_H_

#define MAX_DEVICES 128

/* Video modes */
#define VIDEO_MODE_NONE 0x00
#define VIDEO_MODE_COLOR 0x20
#define VIDEO_MODE_MONO 0x30

/* ioctl helper function */
int ioctl_arg_ok(void* arg, uint sz);

/* Definitions for drivers */
struct IODriver
{
	void* context; /* Pointer to driver specific context */
	slock_t device_lock;
	/* Basic io functions */
	int (*init)(struct IODriver* driver);
	int (*read)(void* dst, uint start_read, uint sz, void* context);
        int (*write)(void* src, uint start_write, uint sz, void* context);
	int (*ioctl)(unsigned long request, void* arg, void* context);
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
struct DeviceDriver* dev_lookup(int dev);

#endif
