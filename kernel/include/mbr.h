#ifndef _MBR_H_
#define _MBR_H_

struct mbr
{
	uint8_t bootstrap[446];
	uint8_t part1[16];
	uint8_t part2[16];
	uint8_t part3[16];
	uint8_t part4[16];
	uint16_t signature;
};

struct mbr_partition_table
{
	uint8_t status;
	uint8_t chs_first[3];
	uint8_t type;
	uint8_t chs_last[3];
	uint32_t start_sector;
	uint32_t sectors;
};

#endif
