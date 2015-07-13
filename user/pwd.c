#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char* argv[])
{
	char buffer[256];
	if(getcwd(buffer, 256) == -1)
	{
		exit(1);
	}
	printf("%s \n", buffer);
	exit(0);
}
