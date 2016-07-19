#include <string.h>
#include "devman.h"
#include "storage.h"
#include "stdlock.h"

static slock_t storagedevice_lock;
static struct StorageDevice storagedevice_table[MAX_STORAGE_DEVICES];

void storagedevice_init(void)
{
	slock_init(&storagedevice_lock);
	memset(storagedevice_table, 0, 
			sizeof(struct StorageDevice) * MAX_STORAGE_DEVICES);
}

struct StorageDevice* storagedevice_alloc(void)
{
	struct StorageDevice* device = NULL;
	slock_acquire(&storagedevice_lock);
	int x;
	for(x = 0;x < MAX_STORAGE_DEVICES;x++)
	{
		if(!storagedevice_table[x].valid)
		{
			memset(storagedevice_table + x, 0, sizeof(struct StorageDevice));
			device = storagedevice_table + x;
			device->valid = 1;
			break;
		}
	}
	slock_release(&storagedevice_lock);

	return device;
}

void storagedevice_free(struct StorageDevice* device)
{
	slock_acquire(&storagedevice_lock);
	device->valid = 0;
	slock_release(&storagedevice_lock);
}
