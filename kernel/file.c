
#ifdef __LINUX__

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#include <stdlib.h>
#include <string.h>

#define _strncpy(dst, src, sz) strlen(strncpy(dst, src, sz))

#else
#include "types.h"
#include "stdarg.h"
#include "stdlib.h"
#define _strncpy strncpy
#endif

int file_path_dir(const char* src, char* dst, uint sz)
{
        if(strlen(src) + 1 >= sz)
                return -1;
        memset(dst, 0, sz);

        int len = _strncpy(dst, src, sz);
        if(dst[len - 1] != '/')
                dst[len] = '/';
        return 0;
}

int file_path_file(const char* src, char* dst, uint sz)
{
        if(strlen(src) == 0) return 1;
        memset(dst, 0, sz);
        int len = _strncpy(dst, src, sz);
        if(!strcmp(dst, "/")) return 0;
        for(;dst[len  - 1] == '/' && len - 1 >= 0;len--)
                dst[len - 1] = 0;
        return 0;
}

int file_path_parent(const char* src, char* dst, uint sz)
{
	memmove(dst, (char*)src, sz);

	/* Delete until we hit a non-slash */
	int x;
	for(x = strlen(src) - 1;x >=0; x--)
	{
		if(src[x] != '/') break;
		dst[x] = 0;
	}

	/* Delete until we hit a slash */
	for(x = strlen(dst) - 1;x >=0; x--)
        {
                if(src[x] == '/') break;
                dst[x] = 0;
        }

	/* Check to see if the path is root */
	if(strlen(dst) == 0) return -1;

	return 0;
}

int file_path_name(const char* src, char* dst, uint sz)
{
        /* Move until we hit a non-slash */
        int x;
        for(x = strlen(src) - 1;x >=0; x--)
                if(src[x] != '/') break;

        /* Move until we hit a slash */
        for(;x >=0; x--)
                if(src[x] == '/') break;

	/* Clear dst */
	memset(dst, 0, sz);

	/* Copy until we hit a slash or the end. */
	int pos;
	for(pos = 0;pos < strlen(src);pos++, x++)
	{
		if(src[x] == '/' || src[x] == 0) break;
		dst[pos] = src[x];
	}

        return 0;
}
