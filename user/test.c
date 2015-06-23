#include "types.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char** argv)
{
	printf("I am a child!\n");
	int fd = open("/file", O_CREATE | O_RDWR, 0);
	if(fd < 0)
	{
		printf("Couldn't open file.\n");
	} else printf("File opened!\n");

	/* Write to file */
	char write_buffer[] = {'h', 'i', 'y', 'a', 0};
	int wrote = write(fd, write_buffer, 4);
	printf("Wrote bytes: %d\n", wrote);

	int seek = lseek(fd, 0, SEEK_SET);
	printf("Seeking: %d\n", seek);

	printf("Reading file...\n");
	char elf_buffer[4];
	int bytes = read(fd, elf_buffer, 4);
	printf("Bytes read: %d\n", bytes);
	int x;
	for(x = 0;x < 4;x++)printf("%d: %c %d\n", x, elf_buffer[x], elf_buffer[x]);
	exit();
}
