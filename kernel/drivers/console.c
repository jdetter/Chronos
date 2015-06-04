#include "types.h"
#include "x86.h"
#include "stdarg.h"
#include "stdlib.h"
#include "console.h"

#define MAX_NUM 15

//uint console_pos = 0;
//void printNextCharacter(char character);
//void printCharacter(uint row, uint col, char character);
//void update_cursor(int pos);

/*int cprintf(char* fmt, ...)
{
	va_list args;
	va_start(&args, (void**)&fmt);
	int arg_pos = 0;
	int x;
	char *s;
	void *pointer;
	int pa;
	for(x = 0;x < strlen(fmt);x++)
	{
		if(fmt[x] == '%')
		{
			char number[MAX_NUM];
			char* number_ptr = number;
			if(fmt[x + 1] == 'd')
			{
				snprintf(number, MAX_NUM, "%d", *((int*)va_arg(args, arg_pos)));
				for(;*number_ptr;number_ptr++)printNextCharacter(*number_ptr);

				arg_pos++;
			} else if(fmt[x + 1] == 'x')
			{
				snprintf(number, MAX_NUM, "%x",*((int*)va_arg(args, arg_pos)));
				for(;*number_ptr;number_ptr++)printNextCharacter(*number_ptr);
				arg_pos++;
			}else if(fmt[x + 1] == 'c')
			{
				printNextCharacter(*((char*)va_arg(args, arg_pos)));
				arg_pos++;
			}else if(fmt[x + 1] == 's')
			{
				s = *((char**)va_arg(args, arg_pos));
				int i;
				for(i = 0; i< strlen(s); i++)
				{
					printNextCharacter(s[i]);
				}
				arg_pos++;
			}else if(fmt[x + 1] == '%')
			{
				printNextCharacter('%');
			}else if(fmt[x + 1] == 'p')
			{
				pointer = *((void**)va_arg(args, arg_pos));
				pa = (int) pointer;
				snprintf(number, MAX_NUM, "%x",  pa);
				for(;*number_ptr;number_ptr++)printNextCharacter(*number_ptr);
				arg_pos++;
			}
			
			x++;			
		} else if(fmt[x] == '\n'){
			
			console_pos = console_pos + (CONSOLE_COLS - (console_pos % CONSOLE_COLS));
		}
		else 
		{
			printNextCharacter(fmt[x]);
		}
	}
	va_end(args);
	return 0;
} */

void console_putc(uint position, char character, char color, uchar colored)
{
	if(colored)
	{
		uchar* vid_addr = CONSOLE_COLOR_BASE 
			+ (position * 2);
		*(vid_addr)     = character;
		*(vid_addr + 1) = color;
	} else {
		uchar* vid_addr = CONSOLE_MONO_BASE
                        + (position);
                *(vid_addr)     = character;
	}
}

void console_init(void)
{
	int pos;
	for(pos=0; pos < CONSOLE_ROWS * CONSOLE_COLS; pos++)
	{
		console_putc(pos, ' ', CONSOLE_DEFAULT_COLOR, 1);
		console_putc(pos, ' ', 0, 0);
	}
}

void console_print_buffer(char* buffer, uchar colored)
{
	uchar* vid_mem = CONSOLE_COLOR_BASE;
	if(!colored)
		vid_mem = CONSOLE_MONO_BASE;
	uint sz = CONSOLE_ROWS * CONSOLE_COLS;
	if(colored) sz *= 2;
	memmove(vid_mem, buffer, sz);
}

//The Index Register is mapped to ports 0x3D5 or 0x3B5.
//The Data Register is mapped to ports 0x3D4 or 0x3B4.
/*
	index offset : 0xE - Cursor Location High
			  	   0xF - Cursor Location Low
	By writing an index offset value into the index Register, it indicates what register the Data Register points to
*/
void console_update_cursor(int pos)
{
	outb(0x3D4, 0xF);
	outb(0x3D5, (uchar) (pos & 0xFF));
	outb(0x3D4, 0xE);
	outb(0x3D5, (uchar) ((pos >> 8)&0xFF)); 
}
