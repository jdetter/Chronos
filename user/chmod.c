#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int moct = 0;
	char* mode = NULL;

	int i;
	for(i = 1;i < argc; i++)
	{
		if(argv[i][0] != '-')
		{
			if(mode == NULL)
			{
				mode = argv[i];
				moct = strtol(mode, NULL, 8);
			} else {
				printf("%s: %d\n", argv[i], moct);
				chmod(argv[i], moct);	
			}
		}
	}

	return 0;
}
