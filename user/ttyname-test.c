#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	printf("malloc version...\n");
	char* name = ttyname(0);
	if(name)
	{
		printf("name: %s\n", name);
	}else printf("FAILURE.\n");

	printf("Buffer version...\n");
	char buff[512];
	ttyname_r(0, buff, 512);
	name = buff;
	if(name)
        {
                printf("name: %s\n", name);
        }else printf("FAILURE.\n");

	return 0;
}
