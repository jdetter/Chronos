/**
 * Author: John Detter <john@detter.com>
 *
 * Kernel debugging / output functions.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "kstdlib.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "stdarg.h"
#include "fsman.h"

extern int video_mode;
inode* log;

/**
 * Hacky version of printf that doesn't require va_args. This method
 * is dangerous if not called properly. This method can be called any time
 * once the kernel has been loaded.
 */
void cprintf(char* fmt, ...)
{
	tty_t t0 = tty_find(0);
	void** argument = (void**)(&fmt + 1);

	int len = strlen(fmt);
	int x;
	for(x = 0;x < len;x++)
	{
		if(fmt[x] == '%' && x + 1 < strlen(fmt))
		{
			if(fmt[x + 1] == '%')
				tty_putc(t0, '%');
			else if(fmt[x + 1] == 'd')
			{
				/* Print in decimal */
				char buffer[32];
				kitoa(*((int*)argument), buffer, 32, 10);
				int y;
				for(y = 0;y < strlen(buffer);y++)
					tty_putc(t0, buffer[y]);
				argument++;
			} else if(fmt[x + 1] == 'p' || fmt[x + 1] == 'x')
			{
				/* Print in hex */
				char buffer[32];
				kitoa(*((int*)argument), buffer, 32, 16);
				int y;
				for(y = 0;y < strlen(buffer);y++)
					tty_putc(t0, buffer[y]);
				argument++;
			} else if(fmt[x + 1] == 'c')
			{
				/* Print character */
				char c = *((char*)argument);
				tty_putc(t0, c);
				argument++;
			} else if(fmt[x + 1] == 's')
			{
				char* str = *((char**)argument);
				int y;
				for(y = 0;y < strlen(str);y++)
					tty_putc(t0, str[y]);
				argument++;
			} else if(fmt[x + 1] == 'b')
			{
				/* Print in binary */
				char buffer[128];
				kitoa(*((int*)argument), buffer, 32, 2);
				int y;
				for(y = 0;y < strlen(buffer);y++)
					tty_putc(t0, buffer[y]);
				argument++;
			}

			x++;
		} else tty_putc(t0, fmt[x]);
	}
}

void tty_erase_display(tty_t t);
void tty_set_cur_rc(tty_t t, int row, int col);
void panic(char* fmt, ...)
{
	char buff[256];
	va_list list;
	va_start(list, fmt);
	vsnprintf(buff, 256, fmt, list);

	cprintf("%s\n", buff);

	fs_sync();
	for(;;);
}
