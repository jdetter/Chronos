#include "types.h"
#include "stdlib.h"
#include "netman.h"

char hostname[HOSTNAME_LEN];

int net_init(void)
{
	strncpy(hostname, "chronos-1.0", HOSTNAME_LEN);
	return 0;
}
