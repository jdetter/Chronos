#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char* argv[])
{
	int i;

	for(i=1; i<argc; i ++)
	{
		create(argv[i], 0);
	}
	exit();
}


