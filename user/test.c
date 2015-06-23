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
	int fd = open("/", O_RDWR, 0);
	if(fd < 0)
	{
		printf("Couldn't open file.\n");
	} else printf("File opened!\n");

	/* read the directory */
	int x;
	for(x = 0;;x++)
	{
		struct directent entry;
		if(readdir(fd, x, &entry) < 0) break;
		printf("%s  %d  %d\n", entry.name, entry.type, entry.inode);
	}	

	exit();
}
