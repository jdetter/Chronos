#ifndef _FSTAB_H_
#define _FSTAB_H_

#include "file.h"

struct fstab
{
	char device[FILE_MAX_PATH];
	char mount[FILE_MAX_PATH];
	int type;
	int options;
	int dump;
	int pass;
};

#define FSTAB_OPT_AUTO 		0x0000
#define FSTAB_OPT_NOAUTO 	0x0001

#define FSTAB_OPT_DEV		0x0000
#define FSTAB_OPT_NODEV		0x0002
#define FSTAB_OPT_EXEC		0x0000
#define FSTAB_OPT_NOEXEC	0x0004
#define FSTAB_OPT_RW		0x0000
#define FSTAB_OPT_RO		0x0008
#define FSTAB_OPT_SYNC		0x0010
#define FSTAB_OPT_ASYNC		0x0000
#define FSTAB_OPT_SUID		0x0000
#define FSTAB_OPT_NOSUID	0x0020
#define FSTAB_OPT_NOUSER	0x0000

#define FSTAB_OPT_USER		0x0040
#define FSTAB_OPT_USERS		0x0080

#define FSTAB_DEFAULTS 		0x0000

/**
 * Parse the system's file system table. The root partition must already
 * be mounted and all disks must be ready.
 */
extern int fstab_init(void);

#endif
