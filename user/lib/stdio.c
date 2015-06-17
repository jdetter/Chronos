#include "types.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdlock.h"
#include "file.h"
#include "chronos.h"

int printf(char* fmt, ...)
{
	va_list list;
	va_start(&list, (void**)&fmt);
	char buffer[256];
	memset(buffer, 0, 256);
	va_snprintf(buffer, 256, &list, fmt);
	int chars = write(1, buffer, strlen(buffer));
	va_end(&list);

	return chars;
}

char getc(void)
{
	char c;
	do
	{
		read(0, &c, 1);
		if(c == 0x0D) c = '\n';
	} while(c == 0x7F);

	return c;
}

int fgets(char* dst, uint sz, uint fd)
{
	return 0;
}

int puts(char* src)
{
	int chars = write(1, src, strlen(src));
	char c = '\n';
	write(1, &c, 1);
	chars++;

	return chars;
}
