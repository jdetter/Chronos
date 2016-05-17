#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dirent.h>

#define __LINUX__
#define __FILE_NO_FUNC__
#include "file.h"
#include "dirent.h"

int file_path_dir(const char* src, char* dst, uint sz)
{
        if(strlen(src) + 1 >= sz)
                return -1;
        memset(dst, 0, sz);
        int len = strlen(strncpy(dst, src, sz));
        if(dst[len - 1] != '/')
                dst[len] = '/';
        return 0;
}

int file_path_file(const char* src, char* dst, uint sz)
{
        if(strlen(src) == 0) return 1;
        memset(dst, 0, sz);
        int len = strlen(strncpy(dst, src, sz));
        if(!strcmp(dst, "/")) return 0;
        for(;dst[len  - 1] == '/' && len - 1 >= 0;len--)
                dst[len - 1] = 0;
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

void usage(void);
void get_perm(struct stat* st, char* dst);

int main(int argc, char* argv[])
{
	char all = 0;
	char list = 0;
	char* path = NULL;
	int x;
	for(x = 1;x < argc;x++)
	{
		if(*argv[x] == '-')
		{
			int y;
			for(y = 1;y < strlen(argv[x]);y++)
			{
				char option = argv[x][y];
				switch(option)
				{
					case 'a':
						all = 1;
						break;
					case 'l':
						list = 1;
						break;
				}
			}
		} else {
			if(path != NULL) usage();
			path = argv[x];
		}
	}

	if(path == NULL) path = (char*)".";

	int fd_dir = open(path, O_RDONLY, 0x0);
	if(fd_dir < 0)
	{
		printf("ls: file not found: %s\n", path);
		exit(1);
	}

	uint size = 0;
	for(x = 0;;x++)
	{
		struct dirent entry;
		if(getdents(fd_dir, &entry, sizeof(struct dirent)) != sizeof(struct chronos_dirent))
			break;
		char entry_path[FILE_MAX_PATH];
		file_path_dir(path, entry_path, FILE_MAX_PATH);
		strncat(entry_path, entry.d_name, FILE_MAX_PATH);
		int file_fd = open(entry_path, O_RDONLY, 0x0);

		if(!all && entry.d_name[0] == '.') continue;

		if(list)
		{
			char out[80];
			memset(out, 0, 80);
			struct stat st;
			fstat(file_fd, &st);
			get_perm(&st, out);
			printf("%s %d:%d %d %s\n", out, 
				st.st_uid, st.st_gid, 
				(int)st.st_size, entry.d_name);
			size += st.st_size;
		} else printf("%s\n", entry.d_name);
		close(file_fd);
	}

	if(list)
	{
		printf("%d files, %d kB\n", 
			x, size / 1024);
	}

	close(fd_dir);

	return 0;
}

void usage(void)
{
	printf("Usage: ls [-al] [path]\n");
	exit(1);
}

void get_perm(struct stat* st, char* dst)
{
	/* Start with type of device */
	switch(st->st_mode & S_IFMT)
	{
		case S_IFREG:
			dst[0] = '-';
			break;
		case S_IFDIR:
			dst[0] = 'd';
			break;
		case S_IFCHR:
			dst[0] = 'c';
			break;
		case S_IFBLK:
			dst[0] = 'b';
			break;
		case S_IFLNK:
			dst[0] = 'l';
			break;
		case S_IFIFO:
			dst[0] = 'f';
			break;
		case S_IFSOCK:
			dst[0] = 's';
			break;
		default:
			dst[0] = '?';
			printf("Unknown property: %u", st->st_mode & S_IFMT);
			break;
	}

	if(st->st_mode & PERM_URD)
		dst[1] = 'r';
	else dst[1] = '-';
	if(st->st_mode & PERM_UWR)
		dst[2] = 'w';
	else dst[2] = '-';
	if(st->st_mode & PERM_UEX)
		dst[3] = 'x';
	else dst[3] = '-';
	/* Check for setuid */
	if((st->st_mode & PERM_UEX) && (st->st_mode & S_ISUID))
		dst[3] = 's';

	if(st->st_mode & PERM_GRD)
		dst[4] = 'r';
	else dst[4] = '-';
	if(st->st_mode & PERM_GWR)
		dst[5] = 'w';
	else dst[5] = '-';
	if(st->st_mode & PERM_GEX)
		dst[6] = 'x';
	else dst[6] = '-';
	/* Check for setgid */
	if((st->st_mode & PERM_GEX) && (st->st_mode & S_ISGID))
                dst[6] = 's';

	if(st->st_mode & PERM_ORD)
		dst[7] = 'r';
	else dst[7] = '-';
	if(st->st_mode & PERM_OWR)
		dst[8] = 'w';
	else dst[8] = '-';
	if(st->st_mode & PERM_OEX)
		dst[9] = 'x';
	else dst[9] = '-';
}
