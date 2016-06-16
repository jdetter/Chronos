#ifdef __LINUX__

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#include <stdlib.h>
#include <string.h>

#define _strncpy(dst, src, sz) strlen(strncpy(dst, src, sz))

#else
#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "stdarg.h"
#define _strncpy strncpy
#endif

const char* file_remove_prefix(const char* path)
{
	if(!*path) return path;

	while(*path && *path == '/') path++;
	if(!*path) return path;
	while(*path && *path != '/') path++;
	return path;
}

int file_path_root(char* path)
{
        while(*path == '/') path++;
        /* As long as we aren't at the end, we're at the root now. */
	while(*path != '/' && *path) path++;
	*path = 0;
        return 0;
}

int file_path_dir(char* path, size_t sz)
{
        if(strlen(path) + 2 >= sz)
                return -1;
	int len = strlen(path);
        if(path[len - 1] != '/')
	{
                path[len] = '/';
		path[len + 1] = 0;
	}

        return 0;
}

int file_path_file(char* path)
{
        if(strlen(path) == 0) return -1;
        if(!strcmp(path, "/")) return 0;
	int len = strlen(path) - 1;
	/* Take all of the slashes off of the end */
	for(;path[len] == '/';len--)
		path[len] = 0;
        return 0;
}

int file_path_parent(char* path)
{
	/* Root has no parent */
	if(!strcmp(path, "/")) return -1;
	/* Delete all slashes on the end */
	if(file_path_file(path)) return -1;
	int end = strlen(path) - 1;
	/* Delete until we hit a slash */
	for(;end > 0 && path[end] != '/' && path[end];end--)
	{
		if(end < 0) return -1;
		path[end] = 0;
	}

	return 0;
}

int file_path_name(char* path)
{
	int end = strlen(path) - 1;
	if(end < 0) return -1;
	if(!strcmp(path, "/")) return 0;

	/* skip over slashes */
	while(end && path[end] == '/') end--;
	
	/* skip non slashes */
	while(end && path[end] != '/' && path[end]) end--;
	if(path[end] == '/') end++;
	
	strncpy(path, path + end, strlen(path));

        return file_path_file(path);
}
