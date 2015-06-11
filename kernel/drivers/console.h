#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define CONSOLE_DEFAULT_COLOR 0x07
#define CONSOLE_ROWS 25
#define CONSOLE_COLS 80
#define CONSOLE_MONO_BASE_ORIG  ((uchar*)0xB0000)
#define CONSOLE_COLOR_BASE_ORIG ((uchar*)0xB8000)

#define CONSOLE_MONO_BASE  ((uchar*)0x20001000)
#define CONSOLE_COLOR_BASE ((uchar*)0x20000000)

/**
 * Print the character at the given row and column. Color specifies the color
 * of the background and foreground. Colored specifies whether or not to
 * print to the colored video memory. If you are printing to mono color, set
 * both color and colored to 0.
 */
void console_putc(uint position, char character, char color, 
	uchar colored, uchar* base_addr);

/**
 * Update the cursor position. 
 */
void console_update_cursor(int pos);

/**
 * Copy to buffer to video memory.
 */
void console_print_buffer(char* buffer, uchar colored);

#endif
