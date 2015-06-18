#ifndef _MBR_H_
#define _MBR_H_

struct mbr
{
	uint_8 bootstrap[446];
	uint_8 part1[16];
	uint_8 part2[16];
	uint_8 part3[16];
	uint_8 part4[16];
	uint_16 signature;
};

struct mbr_partition_table
{
	uint_8 status;
	uint_8 chs_first[3];
	uint_8 type;
	uint_8 chs_last[3];
	uint_32 start_sector;
	uint_32 sectors;
};

#endif
