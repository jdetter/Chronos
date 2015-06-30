#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "panic.h"
#include "serial.h"
#include "console.h"
#include "stdarg.h"
#include "stdmem.h"
#include "stdlib.h"
#include "keyboard.h"
#include "iosched.h"

#define MAX_TTYS 4

struct tty ttys[MAX_TTYS];
static struct tty* active = NULL;

int tty_io_init(struct IODriver* driver);
int tty_io_read(void* dst, uint start_read, uint sz, void* context);
int tty_io_write(void* src, uint start_write, uint sz, void* context);
int tty_io_setup(struct IODriver* driver, uint tty_num)
{
	/* Get the tty */
	tty_t t = tty_find(tty_num);
	struct tty** context = (struct tty**)driver->context;
	*context = t;
	driver->init = tty_io_init;
	driver->read = tty_io_read;
	driver->write = tty_io_write;
	return 0;
}

int tty_io_init(struct IODriver* driver)
{
	driver->valid = 1;
	return 0;
}

int tty_io_read(void* dst, uint start_read, uint sz, void* context)
{
	tty_t t = *(struct tty**)context;
	uchar* dst_c = dst;
	int x;
	for(x = 0;x < sz;x++, dst_c++)
		*dst_c = tty_get_char(t);
	return sz;
}

int tty_io_write(void* src, uint start_write, uint sz, void* context)
{
	tty_t t = *(struct tty**)context;
        uchar* src_c = src;
        int x;
        for(x = 0;x < sz;x++, src_c++)
                tty_print_character(t, *src_c);
        return sz;
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

	/* Zero keyboard status */
	memset(t->keyboard, 0, TTY_KEYBUFFER_SZ);
	t->key_write = 0;
	t->key_read = 0;
	t->key_full = 0;	
	t->key_nls = 0;
}

uint tty_num(tty_t t)
{
	return t->num;
}

void tty_enable(tty_t t)
{
	t->active = 1;
	active = t;
	if(t->type==TTY_TYPE_SERIAL)
	{
		return;
	}
	if(t->type == TTY_TYPE_MONO || t->type == TTY_TYPE_COLOR)
	{
		if(t->display_mode == TTY_MODE_GRAPHIC)
		{
			tty_print_screen(t, (uchar*)t->buffer_graphic);
			console_update_cursor(t->graphic_cursor_pos);
		} else if(t->display_mode == TTY_MODE_TEXT)
		{
			tty_print_screen(t, (uchar*)t->buffer_text);
			console_update_cursor(t->text_cursor_pos);
		}
	}
	else if(t->display_mode==TTY_MODE_GRAPHIC)
	{
		console_print_buffer(t->buffer_graphic, 
			t->type==TTY_TYPE_COLOR,
			t->mem_start);
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
		uchar printable = 1;
		uint new_pos = t->text_cursor_pos;
		int x;
		switch(c)
		{
			case '\n':
				/* Move down a line */
				t->text_cursor_pos += CONSOLE_COLS - 1;
				t->text_cursor_pos -= t->text_cursor_pos %
					CONSOLE_COLS;
				printable = 0;
				break;
			case '\t':
				/* round cursor position */
				new_pos += 8;
				new_pos -= new_pos % 8;
				for(x = 0;t->text_cursor_pos < new_pos;x++)
					tty_print_character(t, ' ');	
				return;
				
		}
		if(t->active && printable)
		{
			console_putc(t->text_cursor_pos, c, t->color, 
					t->type==TTY_TYPE_COLOR, 
					(uchar*)t->mem_start);
		}
		if(printable)
		{
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
			t->text_cursor_pos++;
		}

		if(t->text_cursor_pos >= CONSOLE_COLS * CONSOLE_ROWS)
		{
			tty_scroll(t);
			t->text_cursor_pos = (CONSOLE_COLS * CONSOLE_ROWS)
				- CONSOLE_COLS;
		}

		if(t->active)
			console_update_cursor(t->text_cursor_pos);
	}
	else
	{
		panic("TTY invalid graphics mode");
	}

}

void tty_scroll(tty_t t)
{
	if(!t->type==TTY_TYPE_MONO && !t->type==TTY_TYPE_COLOR)
		return;
	uint bpr = CONSOLE_COLS * 2;
	uint rows = CONSOLE_ROWS;
	char* buffer = t->buffer_text;

	int x;
	for(x = 0;x < rows - 1;x++)
	{
		char* dst_row = buffer + x * bpr;
		char* src_row = dst_row + bpr;
		memmove(dst_row, src_row, bpr);
	}
	memset(buffer + x * bpr, 0, bpr);
	tty_print_screen(t, (uchar*)t->buffer_text);
}

void tty_print_string(tty_t t, char* fmt, ...)
{
	va_list list;
	va_start(&list, (void**)&fmt);
	char buffer[1024];
	memset(buffer, 0, 1024);
	va_snprintf(buffer, 1024, &list, fmt);
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

	va_end(&list);
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
			console_putc(pos, character, t->color,  t->type==TTY_TYPE_COLOR, (uchar*)t->mem_start);
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
		char* vid_addr = (char*)t->mem_start;
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
	char c;
	if(block_keyboard_io(&c, 1) != 1)
		panic("tty: get char failed.\n");
	return c;
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

void tty_delete_char(tty_t t)
{
	char serial_back[] = {0x08, ' ', 0x08};
	switch(t->type)
	{
		default: case TTY_TYPE_NONE: break;
		case TTY_TYPE_COLOR: case TTY_TYPE_MONO:
			switch(t->display_mode)
			{
				case TTY_MODE_NONE: 
				case TTY_MODE_GRAPHIC:
				default:
					return;
				case TTY_MODE_TEXT:
					if(t->text_cursor_pos == 0) break;
					t->text_cursor_pos--;
					tty_print_character(t, ' ');
					t->text_cursor_pos--;
					break;
			}
			/* update the cursor position */
			if(t->active) 
				console_update_cursor(t->text_cursor_pos--);
			break;
		case TTY_TYPE_SERIAL:
			/* Send backspace, space, backspace */
			serial_write(serial_back, 3);
			break;
	}
}	

extern slock_t ptable_lock;
void tty_handle_keyboard_interrupt(void)
{
	if(!active) 
	{
		cprintf("tty: there is no active tty.\n");
		return;
	}

	slock_acquire(&ptable_lock);
	slock_acquire(&active->key_lock);
	char c = 0;
	do
	{
		/* If the buffer is full, break */
		if(active->key_full) 
		{
			cprintf("tty: keyboard buffer is full!\n");
			break;
		}
		switch(active->type)
		{
			default:
				cprintf("tty: active tty is invalid.\n");
				break;
			case TTY_TYPE_COLOR:
			case TTY_TYPE_MONO:
				c = kbd_getc();
				break;
			case TTY_TYPE_SERIAL:
				c = serial_read_noblock();
				break;
		}
		if(!c) break; /* no more characters */
		/* Got character. */

		switch(c)
		{
			/* replace line feed with nl */
			case 13: c = '\n';
				 active->key_nls++;
				 break;

				 /* Delete is special */
				 /* Delete is not a character */
			case 0x7F:
				 /* see if the buffer is empty */
				 if(active->key_read == 
						 active->key_write &&
						 !active->key_full) 
					 continue;

				 int tmp_write = 
					 active->key_write - 1;
				 if(tmp_write < 0)
					 tmp_write = 
						 TTY_KEYBUFFER_SZ - 1;
				 /* Can't delete a nl */
				 if(active->keyboard[tmp_write]
						 == '\n') continue;

				 /* Delete the character */
				 active->keyboard[tmp_write]= 0;
				 tty_delete_char(active);
				 active->key_write = tmp_write;
				 continue;
		}
		/* Echo to tty */
		switch(active->type)
                {
                        default: break;
                        case TTY_TYPE_COLOR:
                        case TTY_TYPE_MONO:
                                tty_print_character(active, c);
                                break;
                        case TTY_TYPE_SERIAL:
                                serial_write(&c, 1);
                                break;
                }

		/* put c into the buffer */
		/* wrap if needed */
		if(active->key_write >= TTY_KEYBUFFER_SZ)
			active->key_write = 0;
		active->keyboard[active->key_write] = c;
		active->key_write++;
		/* update full */
		if(active->key_write == active->key_read)
			active->key_full = 1;
		/* If that was a nl, we want to signal */
		if(c == '\n')
			signal_keyboard_io(active);
	} while(1);

	slock_release(&ptable_lock);
	slock_release(&active->key_lock);
}
