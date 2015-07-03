#ifndef _LIBTTY_H_
#define _LIBTTY_H_

/* Dependant headers */
#include "types.h"

#define TTY_ROWS 25
#define TTY_COLS 80

#define TTY_BLACK 	0x00
#define TTY_BLUE  	0x01
#define TTY_GREEN 	0x02
#define TTY_CYAN  	0x03
#define TTY_RED 	0x04
#define TTY_MAGENTA 	0x05
#define TTY_BROWN 	0x06
#define TTY_LGRAY 	0x07 /* Light gray */

#define TTY_DGRAY	0x08 /* Dark gray */
#define TTY_LBLUE	0x09 /* Light blue */
#define TTY_LGREEN	0x0A /* Light green */
#define TTY_LCYAN	0x0B /* Light cyan */
#define TTY_LRED	0x0C /* Light red */
#define TTY_LMAGENTA	0x0D /* Light magenta */
#define TTY_YELLOW	0x0E
#define TTY_WHITE	0x0F

/**
 * Puts the tty into graphical mode.
 */
void tty_mode_graphic(void);

/**
 * Puts the tty into text mode.
 */
void tty_mode_text(void);

/**
 * Print a string at the given location.
 */
void tty_print_str(int row, int col, char* fmt, ...);

/**
 * Print the character c at the given row and column.
 */
void tty_print_cell(int row, int col, char c);

/**
 * Sets the graphical mode cursor position.
 */
void tty_set_cursor(int row, int col);

/**
 * Clear the screen. Uses the current color as the fill.
 */
void tty_clear_screen(void);

/**
 * Set the forground color (character color).
 */
void tty_set_forground(uchar color);

/**
 * Set the background color.
 */
void tty_set_background(uchar color);

/**
 * Flushes changes to the screen.
 */
void tty_flush(void);

/**
 * Print a text buffer to the screen. A maximum of sz bytes will
 * be read from the buffer and printed to the screen.
 */
int tty_print_text(int row, int col, char* str, uint sz);

#endif
