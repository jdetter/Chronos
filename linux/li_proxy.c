typedef unsigned int uint;

#include "li_proxy.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>


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

int li_strlen(char* str)
{
	return strlen(str);
}

void li_strncpy(char* dst, char* src, unsigned int sz)
{
	strncpy(dst, src, sz);
}

void li_strncat(char* dst, char* src, unsigned int sz)
{
	strncat(dst, src, sz);
}

int li_strcmp(char* str1, char* str2)
{
	return strcmp(str1, str2);
}

int li_strtol(char* str, int len)
{
        return strtol(str, (char**)0, len);
}
