#include <string.h>

#include "fsman.h"

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

	int result;
	while((result = fs_readline(i, file_pos, buffer, 512)) > 0)
	{
		
	}


fstab_init_done:
	fs_close(i);
	return entries;
}
