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
	t->num = num;
	t->active = 0; /* 1: This tty is in the foreground, 0: background*/
	t->type = type;
	t->color = TTY_BACK_BLACK|TTY_FORE_GREY;
	int i;
	for(i = 0; i<TTY_BUFFER_SZ; i+=2)/*sets text and graphics buffers*/
	{
		t->buffer_text[i]= ' ';
		t->buffer_text[i+1]= t->color;
		t->buffer_graphic[i]= ' ';
		t->buffer_graphic[i+1]= t->color;
	}
	t->text_cursor_pos = 0; /* Text mode position of the cursor. */
	t->graphic_cursor_pos = 0; /* Graphic mode position of the cursor. */
	t->text_cursor_enabled= cursor_enabled; /* Whether or not to show the cursor (text)*/
	t->graphical_cursor_enabled=cursor_enabled; /* Whether or not to show the cursor*/
	t->display_mode = TTY_MODE_TEXT; /* Whether we are in text or graphical mode */
	t->mem_start = mem_start; /* The start of video memory. (color, mono only)*/
}

uint tty_num(tty_t t)
{
	return t->num;
}

void tty_enable(tty_t t)
{
	t->active=1;
}

void tty_disable(tty_t t)
{
	t->active=0;
}

void tty_print_character(tty_t t, char c)
{
	//printf(c[t->text_cursor_pos]);
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
	int i;
	for(i=0; i<TTY_BUFFER_SZ; i+=2)
	{
		t->buffer_graphic[i]= ' ';
		t->buffer_graphic[i+1]= t->color;
	}
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
	if(mode==TTY_MODE_TEXT)
	{
		t->text_cursor_enabled = enabled;
	}
	else if(mode==TTY_MODE_GRAPHIC)
	{
		t->graphical_cursor_enabled = enabled;
	}
	return 0;
}

uchar tty_set_cursor_pos(tty_t t, uchar pos, uchar mode)
{
	if(mode==TTY_MODE_TEXT)
	{
		t->text_cursor_pos = pos;
	}
	else if(mode==TTY_MODE_GRAPHIC)
	{
		t->graphic_cursor_pos = pos;
	}
	return 0;
}

void tty_set_color(tty_t t, uchar color)
{
	t->color = color;
}

void tty_set_mode(struct tty* t, uchar mode)
{
	t->display_mode = mode;
}
