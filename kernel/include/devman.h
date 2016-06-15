#ifndef _DEVMAN_H_
#define _DEVMAN_H_

#include <stdlib.h>
#include <stdint.h>

#include "fsman.h"

#define MAX_DEVICES 64

/* Video modes */
#define VIDEO_MODE_NONE 0x00
#define VIDEO_MODE_COLOR 0x20
#define VIDEO_MODE_MONO 0x30

/* ioctl helper function */
extern int ioctl_arg_ok(void* arg, size_t sz);

/* Definitions for drivers */
struct IODevice
{
	int valid; /* Whether or not this IO driver is valid. */
	int type; /* Type of the device */
	char node[FILE_MAX_NAME]; /* Path to where the node is located */
	void* context; /* Pointer to driver specific context */
	slock_t device_lock; /* Kernel level lock for the device */

	/**
	 * Basic ioit function. This is called after the node is
	 * created during boot.
	 */
	int (*init)(struct IODevice* driver);

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

	/**
	 * Manipulate the device. Ususally, on success 0 is returned.
	 */
	int (*ioctl)(unsigned long request, void* arg, void* context);

	/**
	 * Optional methods for checking if the IO will block.
	 */
	int (*ready_read)(void* context);
	int (*ready_write)(void* context);

	/**
	 * Optional method for checking to see the values of different
	 * configuration values (used in pathconf systemcall). Returns
	 * the configuration value if it exists. Returns -1 if the
	 * value doesn't exist.
	 */
	int (*pathconf)(int conf, void* context);
};

/**
 * Some super basic devices
 */
extern struct IODevice* dev_null; /* null device driver */
extern struct IODevice* dev_zero; /* zero device driver */

/**
 * Setup all io drivers for all available devices.
 */
extern int devman_init(void);

/**
 * Populates the dev folder with devices.
 */
extern void dev_populate(void);

/**
 * Allocate a new IO device from the device table. Returns the
 * new device (zeroed). If there aren't any devices left, returns
 * NULL.
 */
extern struct IODevice* dev_alloc(void);

/**
 * Lookup the driver for a device.
 */
extern struct IODevice* dev_lookup(dev_t dev);

/**
 * Create a new direct mapping, starting at the given physical
 * address. The mapping wil have read caching disabled and write
 * through enabled. Returns a pointer to the new mapping, NULL
 * if there isn't enough space left for the mapping.
 */
extern void* dev_new_mapping(uintptr_t phy, size_t sz);

#endif
