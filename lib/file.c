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
