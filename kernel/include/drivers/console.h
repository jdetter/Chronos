#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define CONSOLE_DEFAULT_COLOR 0x07
#define CONSOLE_ROWS 25
#define CONSOLE_COLS 80
#define CONSOLE_MEM_SZ 4000

/**
 * Print the character at the given row and column. Color specifies the color
 * of the background and foreground. Colored specifies whether or not to
 * print to the colored video memory. If you are printing to mono color, set
 * both color and colored to 0.
 */
void console_putc(int position, char character, char color, 
	char colored, char* base_addr);

/**
 * Update the cursor position. 
 */
void console_update_cursor(int pos);

/**
 * Copy to buffer to video memory.
 */
void console_print_buffer(char* buffer, char colored, uintptr_t vid_mem_i);

#endif
