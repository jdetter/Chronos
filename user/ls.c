#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdlock.h"
#include "chronos.h"

void usage(void);
void get_perm(struct file_stat* st, char* dst);

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
		exit();
	}

	uint size = 0;
	for(x = 0;;x++)
	{
		struct directent entry;
		if(readdir(fd_dir, x, &entry))
			break;
		char entry_path[FILE_MAX_PATH];
		file_path_dir(path, entry_path, FILE_MAX_PATH);
		strncat(entry_path, entry.name, FILE_MAX_PATH);
		int file_fd = open(entry_path, O_RDONLY, 0x0);

		if(!all && entry.name[0] == '.') continue;

		if(list)
		{
			char out[80];
			memset(out, 0, 80);
			struct file_stat st;
			fstat(file_fd, &st);
			get_perm(&st, out);
			printf("%s %d:%d %d %s\n", out, 
				st.owner_id, st.group_id, 
				st.sz, entry.name);
			size += st.sz;
		} else printf("%s\n", entry.name);
	}

	if(list)
	{
		printf("%d files, %d kB\n", 
			x, size / 1024);
	}

	exit();
}

void usage(void)
{
	printf("Usage: ls [-al] [path]\n");
	exit();
}

void get_perm(struct file_stat* st, char* dst)
{
	/* Start with type of device */
	switch(st->type)
	{
		case FILE_TYPE_FILE:
			dst[0] = '-';
			break;
		case FILE_TYPE_DIR:
			dst[0] = 'd';
			break;
		case FILE_TYPE_DEVICE:
			dst[0] = 'v';
			break;
		case FILE_TYPE_LINK:
			dst[0] = 'l';
			break;
		case FILE_TYPE_SPECIAL:
			dst[0] = 's';
			break;
	}

	if(st->perm & PERM_URD)
		dst[1] = 'r';
	else dst[1] = '-';
	if(st->perm & PERM_UWR)
		dst[2] = 'w';
	else dst[2] = '-';
	if(st->perm & PERM_UEX)
		dst[3] = 'x';
	else dst[3] = '-';
	if(st->perm & PERM_GRD)
		dst[4] = 'r';
	else dst[4] = '-';
	if(st->perm & PERM_GWR)
		dst[5] = 'w';
	else dst[5] = '-';
	if(st->perm & PERM_GEX)
		dst[6] = 'x';
	else dst[6] = '-';

	if(st->perm & PERM_ORD)
		dst[7] = 'r';
	else dst[7] = '-';
	if(st->perm & PERM_OWR)
		dst[8] = 'w';
	else dst[8] = '-';
	if(st->perm & PERM_OEX)
		dst[9] = 'x';
	else dst[9] = '-';
}
