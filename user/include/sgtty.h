#ifndef _SGTTY_H_
#define _SGTTY_H_

/**
 * BSD style terminal handling. Chronos doesn't actually use these funtions.
 */

struct sgttyb
{
	char sg_ispeed; /* input baud rate */
	char sg_ospeed; /* output baud rate */
	char sg_erase; /* erase character */
	char sg_kill; /* kill character */
	int sg_flags; /* various flags */
};

int gtty(int fd, struct sgttyb* param);
int stty(int fd, struct sgttyb* param);

#endif
