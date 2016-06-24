#ifndef _MBR_H_
#define _MBR_H_

#include <stdint.h>

struct mbr_partition
{
	uint8_t status;
	uint8_t chs_first[3];
	uint8_t type;
	uint8_t chs_last[3];
	uint32_t start_sector;
	uint32_t sectors;
};


struct __attribute__ ((__packed__)) master_boot_record
{
	uint8_t bootstrap[446];
	struct mbr_partition table[4];
	uint16_t signature;
};

#endif
