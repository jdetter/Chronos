#include "types.h"
#include "li_proxy.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdarg.h>

void* li_mmap(void* a, uint l, int p, int fl, int fd, uint o)
{
	return mmap(a, l, p, fl, fd, o);
}

void li_printf(const char *format , ... )
{
	va_list arglist;
	va_start(arglist, format);
	vprintf(format, arglist);
	va_end(arglist);
}
