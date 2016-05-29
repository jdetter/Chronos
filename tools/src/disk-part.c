#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

/* Partition a disk */

/**
 * Options:
 * -p <n>,<l>,<s>,<t>
 *     n: The partition number (integer)
 *     l: The logical start of the partition (integer)
 *     s: How many sectors are in the partition (integer)
 *     t: The type of the partition (integer)
 *
 *
 * example:
 *  disk-part -p 1,64,4096,7 -p 2,6000,1024,6 disk.img
 *
 *  ^ This will create 2 partitions: one partition starting at sector 64 that
 *         contains 4096 sectors with type 7 (NTFS) and one partition starting 
 * 	   at sector 6000 that contains 1024 sectors. With type 6 (FAT16)
 *	   and it will use disk.img as the disk to configure.
 *
 * NOTE: use classical generic MBR formatting:
 * - https://en.wikipedia.org/wiki/Master_boot_record#Sector_layout
 *
 * NOTE: if a partition is not specified, its entry  should be 
 * cleared (with all 0s).
 *
 */
char* nextInt(char* ptr){
	for(;*ptr&&*ptr!=','; ptr++);
	return ptr;
}

int main(int argc, char** argv)
{

	typedef struct partition{
		char boot;
		char reserved_1 [3];
		char type;
		char reserved_2 [3];
		unsigned int first_sect;
		unsigned int num_of_sects;
	}partition;
	
	partition table [4];
	char* disk = NULL;
	memset(table, 0, sizeof(partition)*4);
	
	int i;
	for(i =1 ; i<argc ; i++){
		if(argv[i][0] == '-' && argv[i][1]== 'p'){
			char *config = argv[i+1];
			int part_num = atoi(config);
			config = nextInt(config);
			if(!*config){
				printf("disk-part: Invalid partition type");
				return -1;
			}
			int part_start = atoi(config);
			config = nextInt(config);
			if(!*config){
				printf("disk-part: Invalid partition type");
				return -1;
			}
			int num_sects = atoi(config);
			config = nextInt(config);
			if(!*config){
				printf("disk-part: Invalid partition type");
				return -1;
			}
			int part_type = atoi(config);	
			table[part_num].first_sect = part_start;
			table[part_num].num_of_sects = num_sects;
			table[part_num].type = part_type;
		}else{
			if(disk){
				printf("disk-part: invalid options");
				return -1;
			}
				
			disk = argv[i];
		
		}
		
		
		
		
	}
	int fd = open(disk, O_RDONLY);
		if(fd < 0){
			
			printf("disk-part: bad file\n");
			return -1;
		}
	char buff [512];
		read(fd, buff, 512);
		memmove(buff+446, table, sizeof(partition)*4);
		lseek(fd, 0, SEEK_SET);
		write(fd, buff, 512);
		printf("successful partition table\n");
	
	
	return 0;
}
