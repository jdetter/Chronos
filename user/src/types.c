#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <time.h>
#include <_ansi.h>
#include <sys/ioctl.h>

#define __LINUX__
#include "file.h"

int main(int argc, char** argv)
{
	printf("dev: %lu\n", sizeof(dev_t));
	printf("ino: %lu\n", sizeof(ino_t));
	printf("mode: %lu\n", sizeof(mode_t));
	printf("nlink: %lu\n", sizeof(nlink_t));
	printf("uid: %lu\n", sizeof(uid_t));
	printf("gid: %lu\n", sizeof(gid_t));
	printf("off: %lu\n", sizeof(off_t));
	
	printf("\ntime_t: %lu\n", sizeof(time_t));
	printf("\nstruct stat: %lu\n", sizeof(struct stat));
	return 0;
}
