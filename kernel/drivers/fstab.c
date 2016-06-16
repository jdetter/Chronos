#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "kstdlib.h"
#include "fsman.h"
#include "fstab.h"
#include "panic.h"

#define FSTAB_COUNT		16
struct fstab fstab_table[FSTAB_COUNT];

int fstab_init(void)
{
	memset(fstab_table, 0, sizeof(struct fstab) * FSTAB_COUNT);

	/* Parse the filesystem table */
	inode i = fs_open("/etc/fstab", O_RDONLY, 0000, 0x0, 0x0);

	/* Make sure we were able to open the file */
	if(!i)
		return -1;

	int entries = 0;

	size_t file_pos = 0;
	char buffer[512];

	int line_num = 0;
	int result;
	while((result = fs_readline(i, file_pos, buffer, 512)) > 0)
	{
		line_num++;
		file_pos += result;
		trim(buffer);

		/* Does this start with a #? */
		if(buffer[0] == '#')
			continue;

		/* Is this line blank? */
		if(strlen(buffer) <= 1)
			continue;

		/* Parse the entry */
		struct fstab* entry = fstab_table + entries;

		char* strtok_state;
		int entry_num;
		for(entry_num = 0;entry_num < 6;entry_num++)
		{
			char* start = buffer;
			if(entry_num != 0)
				start = NULL;
			
			char* argument = strtok_r(start, " \t", &strtok_state);
			if(!argument) break;

			switch(entry_num)
			{
				case 0: /* Device or UUID */
					strncpy(entry->device, argument, sizeof(entry->device));
					break;
				case 1: /* Mount point on file system */
					strncpy(entry->mount, argument, sizeof(entry->mount));
					break;
				case 2: /* Type of partition */
					/* TODO: CONVERT THE FS STRING TO A TYPE ID */
					break;
				case 3:
					/* TODO: comma seperated list of options */
					break;
				case 4:
					/* TODO: dump number */
					break;
				case 5:
					/* TODO: pass number */
					break;
				default:
					panic("kernel: Logic error! fstab.c\n");
					break;
			}
		}

		if(entry_num != 6)
		{
			cprintf("kernel: warning: syntax issue at fstab:%d\n", line_num);
			continue;
		}

		entries++;
	}

	fs_close(i);
	return entries;
}
