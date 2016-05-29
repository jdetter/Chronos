#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
	uint uid = 0;
	uint gid = 0;
	int mode = 0;

	int i;
	for(i = 1;i < argc; i++)
	{
		if(argv[i][0] != '-')
		{
			if(mode == 0)
			{
				mode = 1;
				/* See if there is a colon */
				char* grp = NULL;
				int y;
				int len = strlen(argv[i]);
				for(y = 0;y < len;y++)
				{
					if(argv[i][y] == ':')
					{
						argv[i][y] = 0;
						y++;
						grp = argv[i] + y;
						break;
					}
				}

				if(grp != NULL)
				{
					uid = atoi(argv[i]);
					gid = atoi(grp);
				} else {
					uid = gid = atoi(argv[i]);
				}
			} else {
				chown(argv[i], uid, gid);	
			}
		}
	}

	return 0;	
}
