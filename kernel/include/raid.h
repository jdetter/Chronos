#ifndef _RAID_H_
#define _RAID_H_

/**
 * Initilize all of the raid drivers. Only needs to be called once on
 * boot and must be called before any raid drivers are initlized. Returns
 * 0 on success.
 */
void raid_init(void);

/**
 * Initliize the raid driver with the array of drivers. driver_count is the
 * amount of drivers in the array. Returns 0 on success.
 */
struct StorageDevice* raid0_init(struct StorageDevice** drivers,
	int driver_count, int stride);

/**
 * Initilize the raid driver with the array of drivers. driver_count is the
 * amount of drivers in the array. Returns 0 on success.
 */
int raid0_setup_cache(struct FSDriver* driver, struct StorageDevice* raid);

/**
 * Initilize the raid driver with the array of drivers. driver_count is the
 * amount of drivers in the array. Returns 0 on success.
 */
struct StorageDevice* raid1_init(struct StorageDevice** drivers,
	int driver_count, int stride);

/**
 * Setup an FSDriver to use the raid specific disk cache. Returns 0
 * on success, nonzero otherwise.
 */
int raid1_setup_cache(struct FSDriver* driver, struct StorageDevice* raid);

/**
 * Initilize the raid driver with the array of drivers. driver_count is the
 * amount of drivers in the array. Returns 0 on success.
 */
struct StorageDevice* raid5_init(struct StorageDevice** drivers,
        int driver_count, int stride);

/**
 * Setup an FSDriver to use the raid specific disk cache. Returns 0
 * on success, nonzero otherwise.
 */
int raid5_setup_cache(struct FSDriver* driver, struct StorageDevice* raid);

#endif
