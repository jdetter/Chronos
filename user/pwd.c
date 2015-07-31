#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	char buffer[256];
	if(getcwd(buffer, 256) == NULL)
		return 1;
	printf("%s \n", buffer);
	return 0;
}
