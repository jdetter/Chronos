#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include "mbr.h"

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("USAGE: ptabldump <device>\n");
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		perror("ptabledump");
		return 1;
	}

	assert(sizeof(struct mbr_partition) == 16);
	assert(sizeof(struct master_boot_record) == 512);

	struct master_boot_record mbr;
	memset(&mbr, 0, sizeof(struct master_boot_record));

	char* dst = (char*)&mbr;
	int total_read = sizeof(struct master_boot_record);
	int r = 0;
	int ret;

	while(r < total_read && (ret = read(fd, dst + r, total_read - r)) != 0)
	{
		if(ret < 0)
		{
			printf("READ FAULT\n");
			return 1;
		}

		r += ret;
	}

	if(r != sizeof(struct master_boot_record))
	{
		printf("Read incomplete!!\n");
		return 1;
	}

	struct mbr_partition* table = mbr.table;

	/* Print partitions */
	int x;
	for(x = 0;x < 4;x++)
	{
		if(x != 0) printf("\n");
		printf("Partition %d\n", x + 1);

		int c1 = table[x].chs_first[2] | 
			((table[x].chs_first[1] & 0xC0) << 2);
		int h1 = table[x].chs_first[0];
		int s1 = (table[x].chs_first[1] & 0x3F);
		int c2 = table[x].chs_last[2] | 
			((table[x].chs_last[1] & 0xC0) << 2);
		int h2 = table[x].chs_last[0];
		int s2 = (table[x].chs_last[1] & 0x3F);

		printf("\tStatus:     0x%x\n", table[x].status);
		printf("\tFS Type:    0x%x\n", table[x].type);
		printf("\tCHS first:\n");
		printf("\t\tCylindar: 0x%x\n", c1);
		printf("\t\tHead:     0x%x\n", h1);
		printf("\t\tSector:   0x%x\n", s1);
		printf("\tCHS last:\n");
		printf("\t\tCylindar: 0x%x\n", c2);
		printf("\t\tHead:     0x%x\n", h2);
		printf("\t\tSector:   0x%x\n", s2);
		printf("\tStart sector: 0x%x\n", table[x].start_sector);
		printf("\tSector count: 0x%x\n", table[x].sectors);
	}

	return 0;
}
