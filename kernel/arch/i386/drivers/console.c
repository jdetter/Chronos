/**
 * Author: Amber Arnold <alarnold2@wisc.edu>
 * 
 * Functions for printing characters to a terminal screen.
 *
 */
#include <stdlib.h>
#include <string.h>

#include "x86.h"
#include "stdarg.h"
#include "k/drivers/console.h"

#define MAX_NUM 15

void console_putc(int position, char character, char color, char colored, char* base_addr)
{
	if(colored)
	{
		char* vid_addr = base_addr
			+ (position * 2);
		*(vid_addr)     = character;
		*(vid_addr + 1) = color;
	} else {
		char* vid_addr = base_addr
                        + (position);
                *(vid_addr)     = character;
	}
}

void console_print_buffer(char* buffer, char colored, uintptr_t vid_mem_i)
{
	char* vid_mem = (char*)vid_mem_i;
	int sz = CONSOLE_ROWS * CONSOLE_COLS;
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
