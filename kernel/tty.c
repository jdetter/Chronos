#include "types.h"
#include "tty.h"
#include "panic.h"

#define MAX_TTYS 4

struct tty ttys[MAX_TTYS];

tty_t tty_find(uint index)
{
	if(index >= MAX_TTYS) return NULL;
	return ttys + index;
}

void tty_init(tty_t t, uint num, uchar type, uint cursor_enabled, 
	uint mem_start)
{

}

uint tty_num(tty_t t)
{
	return 0;
}

void tty_enable(tty_t t)
{

}

void tty_disable(tty_t t)
{

}

void tty_print_character(tty_t t, char c)
{

}

void tty_print_string(tty_t t, char* fmt, ...)
{

}

void tty_print_cell(tty_t t, uint row, uint col, uint character)
{

}

void tty_print_screen(tty_t t, uchar* buffer)
{

}

void tty_clear_graphic(tty_t t)
{

}

char tty_get_char(tty_t t)
{
	return 0;
}

uchar tty_get_key(tty_t t)
{
	return 0;
}

uchar tty_set_cursor(tty_t t, uchar enabled, uchar mode)
{
	return 0;
}

uchar tty_set_cursor_pos(tty_t t, uchar pos, uchar mode)
{
	return 0;
}

void tty_set_color(tty_t t, uchar color)
{

}

void tty_set_mode(struct tty* t, uchar mode)
{

}
