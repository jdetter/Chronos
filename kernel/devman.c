#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "pipe.h"
#include "device.h"
#include "vm.h"
#include "panic.h"

static slock_t device_table_lock;
static struct IODevice devices[MAX_DEVICES];
static uintptr_t curr_mapping;

static int io_null_init(struct IODevice* driver);
static int io_zero_init(struct IODevice* driver);

struct IODevice* dev_null; /* null device driver */
struct IODevice* dev_zero; /* zero device driver */

/** Architecture device setup function */
extern int dev_init();

struct IODevice* dev_alloc(void)
{
	struct IODevice* device = NULL;
	int x;

	slock_acquire(&device_table_lock);
	for(x = 1;x < MAX_DEVICES;x++)
	{
		if(devices[x].valid == 0)
		{
			device = devices + x;
			memset(device, 0, sizeof(struct IODevice));
			device->valid = 1;
			break;
		}
	}
	slock_release(&device_table_lock);

	return device;
}

void* dev_new_mapping(uintptr_t phy, size_t sz)
{
	if(!curr_mapping)
		return NULL;

	vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
	/* Disable caching and enable write through */
	vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT
		| VM_TBL_CACH | VM_TBL_WRTH;

	uintptr_t v_start = curr_mapping;
	uintptr_t v_end = PGROUNDUP(v_start + sz);
	if(v_end > KVM_HARDWARE_E)
		panic("devman: Out of hardware memory.\n");

	uintptr_t x;
	for(x = 0;x < sz;x += PGSIZE)
	{
		vm_mappage(phy + x, v_start + x, k_pgdir,
				dir_flags, tbl_flags);
	}

	curr_mapping = v_end;
	return (void*)v_start;
}

void dev_populate(void)
{
	int dev_perm = PERM_URD | PERM_UWR |
		PERM_GRD | PERM_GWR |
		PERM_ORD | PERM_OWR;

	dev_t x;
	for(x = 0;x < MAX_DEVICES;x++)
	{
		if(!devices[x].valid) continue;
		dev_t type = devices[x].type;
		switch(type)
		{
			/* Character devices */
			case DEV_IO:
			case DEV_TTY:
			case DEV_COM:
				type = S_IFCHR;
				break;
				/* Block devices */
			case DEV_DISK_PART:
			case DEV_DISK:
			case DEV_RAM:
			case DEV_LOOP:
				type = S_IFBLK;
				break;
			case DEV_PIPE:
				type = S_IFIFO;
				break;
			case DEV_SOCK:
				type = S_IFSOCK;
				break;
			default:
				type = 0;
				// cprintf("Invalid device: %d\n", x);
				break;
		}
		if(!type) continue;

		fs_mknod(devices[x].node, x, devices[x].type,
				dev_perm | type);
	}
}

struct IODevice* dev_lookup(dev_t dev)
{
	if(dev > 0 && dev < MAX_DEVICES && devices[dev].valid)
		return devices + dev;
	else return NULL;
}

int io_null_read(void* dst, fileoff_t start_read, size_t sz, void* context)
{
	return 0;
}

int io_null_write(void* dst, fileoff_t start_read, size_t sz, void* context)
{
	return sz;
}

int io_null_init(struct IODevice* device)
{
	device->init = io_null_init;
	device->read = io_null_read;
	device->write = io_null_write;
	device->ioctl = NULL;
	return 0;
}

int io_zero_read(void* dst, fileoff_t start_read, size_t sz, void* context)
{
	memset(dst, 0, sz);
	return sz;
}

int io_zero_write(void* dst, fileoff_t start_read, size_t sz, void* context)
{
	return sz;
}

int io_zero_init(struct IODevice* device)
{
	device->init = io_zero_init;
	device->read = io_zero_read;
	device->write = io_zero_write;
	device->ioctl = NULL;
	return 0;
}

int devman_init()
{
    /* Setup the drive table */
    curr_mapping = KVM_HARDWARE_S;
    slock_init(&device_table_lock);
    memset(&devices, 0, sizeof(struct IODevice) * MAX_DEVICES);

    /* Call the architecture setup function */
    if(dev_init())
        panic("kernel: Architecture device setup has failed.\n");

    /* make null and zero devices */
	struct IODevice* device = dev_alloc();
    device->type = DEV_IO;
    snprintf(device->node, FILE_MAX_PATH, "/dev/null");
    device->init = io_null_init;
    dev_null = device;

    device = dev_alloc();
    device->type = DEV_IO;
    snprintf(device->node, FILE_MAX_PATH, "/dev/zero");
    device->init = io_zero_init;
    dev_zero = device;

    /* Do final init on all io devices */
	dev_t x;
    for(x = 0;x < MAX_DEVICES;x++)
        if(devices[x].valid)
            devices[x].init(devices + x);

    return 0;
}
