#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	if(argc<2)
	{
		printf("Usage: cat [-n] [file]\n");
		exit(1);
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
				exit(1);
			}
			path = argv[1];
		}
	}

	int fileDescript = open(path, O_RDONLY);
	if(fileDescript==-1)
	{
		printf("cat: File does not exist.\n");
		exit(1);
	}

#define BUFF_SIZE 512
	struct stat st;
	if(fstat(fileDescript, &st)) return -1;
	int fileLength = st.st_size;

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
		if(write(1, buffer, left) != left)
			return -1;
		i += left;
	}

	return 0;
}
