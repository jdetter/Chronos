#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "netman.h"

char hostname[HOSTNAME_LEN];

int net_init(void)
{
	strncpy(hostname, "chronos-1.0", HOSTNAME_LEN);
	return 0;
}
