/**
 * boot-sign takes as an argument the file name of a 512 byte file that is
 * ready to be signed. This boot-sign should return a non zero value if
 * any error is encountered.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	printf("*************** Boot Sign Utility ***************\n");
	off_t size;
	int fd;
	if(argc!=2){
		return 1;
	}
	fd = open(argv[1], O_RDWR);
	assert(fd > -1);
	size = lseek(fd, 0, SEEK_END); /*offset from beginning to end in bytes*/
	lseek(fd, 0, SEEK_SET); /*back to beginning of file*/
	if(size<510){
		printf("Boot sector size: %d\n", (int)size);
		char val[] = {0x55, 0xAA};
		lseek(fd, 510, SEEK_SET); /* put 0x55 at 510 bytes from beginning*/
		write(fd, val , 2);
	}else if(size == 512){
		char sign[2];
		lseek(fd, 510, SEEK_SET);
		read(fd, sign, 2);
		if(sign[0] == (char)0x55 && sign[1] == (char)0xAA)
		{
			printf("Boot sector is already signed.\n");
			return 0;
		}

		printf("ERROR: Boot sector is too large!\n");
		return 1;
	} else {
		printf("Error: Boot sector is too large!\n");
		printf("Size: %d bytes\n", (int)size);
		return 1;
	}

	printf("Boot signature complete.\n");
	close(fd);
	return 0;
}
