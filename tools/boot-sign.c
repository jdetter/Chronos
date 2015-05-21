/**
 * boot-sign takes as an argument the file name of a 512 byte file that is
 * ready to be signed. This boot-sign should return a non zero value if
 * any error is encountered.
 */
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
	off_t size;
	int fd;
	if(argc!=2){
		return 1;
	}
	fd = open(argv[1], O_RDWR);
	assert(fd>-1);
	size = lseek(fd, 0, SEEK_END); /*offset from beginning to end in bytes*/
	lseek(fd, 0, SEEK_SET); /*back to beginning of file*/
	if(size<510){
		int val = 0x55;
		lseek(fd, 510, SEEK_CUR); /* put 0x55 at 510 bytes from beginning*/
		write(fd, &val ,1);
		lseek(fd, 1, SEEK_CUR); /* put 0xAA at 511 bytes from beginning*/
		val = 0xAA
		write(fd, &val, 1);
	}else{
		return 1;
	}
	close(fd);
	return 0;
}
