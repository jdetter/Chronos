#include "types.h"
#include "stdarg.h"
#include "stdlib.h"

int file_path_dir(const char* src, char* dst, uint sz)
{
        if(strlen(src) + 1 >= sz)
                return -1;
        memset(dst, 0, sz);
        int strlen = strncpy(dst, src, sz);
        if(dst[strlen - 1] != '/')
                dst[strlen] = '/';
        return 0;
}

int file_path_file(const char* src, char* dst, uint sz)
{
        if(strlen(src) == 0) return 1;
        memset(dst, 0, sz);
        int strlen = strncpy(dst, src, sz);
        if(!strcmp(dst, "/")) return 0;
        for(;dst[strlen  - 1] == '/' && strlen - 1 >= 0;strlen--)
                dst[strlen - 1] = 0;
        return 0;
}

int file_path_parent(const char* src, char* dst)
{
	memmove(dst, (char*)src, strlen(src) + 1);

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
