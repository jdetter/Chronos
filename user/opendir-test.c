#include <stdio.h>
#include "dirent.h"
#include <sys/types.h>

int main(int argc, char** argv)
{
	DIR* d;
	d = opendir("/");

	closedir(d);
	return -1;
}
