#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define CONSOLE_DEFAULT_COLOR 0x07
#define CONSOLE_ROWS 25
#define CONSOLE_COLS 80
//#define CONSOLE_COLOR_BASE ((char*)0xB0000)
#define CONSOLE_COLOR_BASE ((char*)0xB8000)

/**
 * Print the string to the terminal. This uses the same sort of format
 * parsing that printf uses. Returns the amount of characters printed to
 * the screen.
 */
int cprintf(char* fmt, ...);

/**
* Sets up video memory
*/
void cinit(void);

#endif
