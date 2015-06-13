#include "types.h"
#include "tty.h"
#include "panic.h"
#include "serial.h"
#include "console.h"
#include "stdarg.h"
#include "stdmem.h"
#include "stdlib.h"

#define MAX_TTYS 4

struct tty ttys[MAX_TTYS];

/**
 * Hacky version of printf that doesn't require va_args. This method
 * is dangerous if not called properly. This method can be called any time
 * once the kernel has been loaded.
 */
void cprintf(char* fmt, ...)
{
	if(ttys[0].type == 0)
	{
		tty_init(ttys + 0, 0, TTY_TYPE_SERIAL, 0, 0);
		tty_enable(ttys);
	}

	void** argument = (void**)(&fmt + 1);

	uint x;
	for(x = 0;x < strlen(fmt);x++)
	{
		if(fmt[x] == '%' && x + 1 < strlen(fmt))
		{
			if(fmt[x + 1] == '%')
				tty_print_character(&ttys[0], '%');
			else if(fmt[x + 1] == 'd')
			{
				/* Print in decimal */
				char buffer[32];
				itoa(*((int*)argument), buffer, 32, 10);
				uint y;
				for(y = 0;y < strlen(buffer);y++)
					tty_print_character(ttys, buffer[y]);
				argument++;
			} else if(fmt[x + 1] == 'p' || fmt[x + 1] == 'x')
			{
				/* Print in hex */
				char buffer[32];
                                itoa(*((int*)argument), buffer, 32, 16);
                                uint y;
                                for(y = 0;y < strlen(buffer);y++)
                                        tty_print_character(ttys, buffer[y]);
				argument++;
			} else if(fmt[x + 1] == 'c')
			{
				/* Print character */
				char c = *((char*)argument);
				tty_print_character(ttys, c);
				argument++;
			} else if(fmt[x + 1] == 's')
			{
				char* str = *((char**)argument);
				int y;
				for(y = 0;y < strlen(str);y++)
					tty_print_character(ttys, str[y]);
				argument++;
			} else if(fmt[x + 1] == 'b')
			{
				/* Print in binary */
				char buffer[128];
                                itoa(*((int*)argument), buffer, 32, 2);
                                uint y;
                                for(y = 0;y < strlen(buffer);y++)
                                        tty_print_character(ttys, buffer[y]);
                                argument++;
			}

			x++;
		} else tty_print_character(ttys, fmt[x]);
	}
}

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
	t->active = 1;
	if(t->type==TTY_TYPE_SERIAL)
	{
		return;
	}
	if(t->type == TTY_TYPE_MONO || t->type == TTY_TYPE_COLOR)
	{
		if(t->display_mode == TTY_MODE_GRAPHIC)
		{
			console_print_buffer(t->buffer_graphic, 
					t->type==TTY_TYPE_COLOR);
			console_update_cursor(t->graphic_cursor_pos);
		} else if(t->display_mode == TTY_MODE_TEXT)
		{
			console_print_buffer(t->buffer_graphic, 
					t->type==TTY_TYPE_COLOR);
			console_update_cursor(t->text_cursor_pos);
		}
	}
	else if(t->display_mode==TTY_MODE_GRAPHIC)
	{
		console_print_buffer(t->buffer_graphic, t->type==TTY_TYPE_COLOR);
	}
	else
	{
		panic("TTY invalid graphics mode");
	}
}
void tty_disable(tty_t t)
{
	t->active=0;
}

void tty_print_character(tty_t t, char c)
{
	if(t->type==TTY_TYPE_SERIAL)
	{
		if(t->active) serial_write(&c,1);
		return;
	}
	if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		if(t->active)
		{
			console_putc(t->text_cursor_pos, c, t->color, t->type==TTY_TYPE_COLOR);
		}
		if(t->type==TTY_TYPE_COLOR)
		{
			char* vid_addr = t->buffer_text
				+ (t->text_cursor_pos * 2);
			*(vid_addr)     = c;
			*(vid_addr + 1) = t->color;
		}
		else
		{
			char* vid_addr = t->buffer_text
				+ (t->text_cursor_pos);
			*(vid_addr)     = c;
		}
	}
	else
	{
		panic("TTY invalid graphics mode");
	}

}


void tty_print_string(tty_t t, char* fmt, ...)
{
	va_list list;
	va_start(&list, (void**)&fmt);
	char buffer[1024];
	va_snprintf(buffer, 1024, list, fmt);
	if(t->type==TTY_TYPE_COLOR||t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_SERIAL)
	{
		int i;
		for(i=0; i <strlen(buffer); i++)
		{
			tty_print_character(t, buffer[i]);
		}
	}
	else
	{
		panic("TTY invalid graphics mode");
	}
}

void tty_print_cell(tty_t t, uint row, uint col, uint character)
{
	if(t->type==TTY_TYPE_SERIAL)
	{
		tty_print_character(t, character);
	}
	else if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		uint pos =(row*CONSOLE_COLS)+col;
		if(t->type==TTY_TYPE_COLOR)
		{
			char* vid_addr = t->buffer_graphic
				+ (t->graphic_cursor_pos * 2);
			*(vid_addr)     = character;
			*(vid_addr + 1) = t->color;
		}
		else
		{
			char* vid_addr = t->buffer_graphic
				+ (t->graphic_cursor_pos);
			*(vid_addr)     = character;
		}
		if(t->active)
		{
			console_putc(pos, character, t->color,  t->type==TTY_TYPE_COLOR);
		}

	}
	else
	{
		panic("TTY invalid graphics mode");
	}

}

void tty_print_screen(tty_t t, uchar* buffer)
{
	if(t->type==TTY_TYPE_SERIAL)
	{
		return;
	}
	else if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		char* vid_addr = t->buffer_graphic;
		//char* graph_addr = NULL;
		uint sz = 0;
		if(t->type==TTY_TYPE_COLOR)
		{
			//graph_addr = CONSOLE_COLOR_BASE;
			sz = CONSOLE_ROWS*CONSOLE_COLS*2;
		}
		else
		{
			//graph_addr = CONSOLE_MONO_BASE;
			sz = CONSOLE_ROWS*CONSOLE_COLS;
		}
		memmove(vid_addr, buffer, sz);
		if(t->active)
		{
			//memmove(graph_addr, buffer, sz);
		}

	}
	else
	{
		panic("TTY invalid graphics mode");
	}
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
	if(t->type == TTY_TYPE_SERIAL)
	{
		char c;
		serial_read(&c, 1);
		return c;
	} else if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{

	}


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
