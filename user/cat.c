#include "types.h"
#include "stdlib.h"
#include "file.h"
#include "chronos.h"
#include "stdio.h"
#include "stdarg.h"

int main(int argc, char** argv)
{
	if(argc<2)
	{
		printf("Usage: cat [-n] [file]\n");
		exit();
	}
	char* path= NULL;
	int x;
	for(x = 1;x < argc;x++)
	{
		if(strcmp(argv[x], "-n") == 0)
		{
		} else {
			if(path)
			{
				printf("Usage: cat [-n] [file]\n");
				exit();
			}
			path = argv[1];
		}
	}

	int fileDescript = open(path, O_RDONLY, 0);
	if(fileDescript==-1)
	{
		printf("cat: File does not exist.\n");
		exit();
	}

	#define BUFF_SIZE 512
	int fileLength = lseek(fileDescript, 0, SEEK_END);
	/* reset seek */
	lseek(fileDescript, 0, SEEK_SET);
	char buffer[BUFF_SIZE];
	int i;
	for(i=0; i<fileLength;)
	{
		int left = fileLength - i;
		if(left > BUFF_SIZE - 1) left = BUFF_SIZE - 1;
		read(fileDescript, buffer, left);
		buffer[BUFF_SIZE - 1] = 0;
		printf(buffer);
		i += left;
	}

	exit();
}
