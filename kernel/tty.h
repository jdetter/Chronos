#ifndef _TTY_H_
#define _TTY_H_

/**
 * Maximum number of ttys that can be connected
 */
#define MAX_TTYS 4

/** 
 * For a color monitor, there are 80 rows and 25 columns and each position
 * is 2 bytes, one byte is the character and one byte is the color. See the
 * color specification below.
 */
#define TTY_BUFFER_SZ 4000

/**
 * The amount of characters the keyboard buffer can hold.
 */
#define TTY_KEYBUFFER_SZ 0x100

/**
 * The types of ttys. The type of the tty determines how it communicates
 * to the hardware. Color and mono ttys will write to video memory, serial ttys
 * will read and write from the serial port.
 */
#define TTY_TYPE_NONE		0x0 /* This tty is disabled. */
#define TTY_TYPE_COLOR		0x1 /* This tty supports VGA color. */
#define TTY_TYPE_MONO		0x2 /* This tty supports VGA text (no color).*/
#define TTY_TYPE_SERIAL		0x3 /* This tty supports serial io. */

/**
 * The tty modes. The mode determines the data that is displayed on the screen.
 */
#define TTY_MODE_NONE		0x0 /* This tty is disabled. */
#define TTY_MODE_TEXT		0x1 /* Display the data in the text buffer. */
#define TTY_MODE_GRAPHIC 	0x2 /* Display the data in the graphic buffer.*/

/**
 * All of the available foreground and background colors for the tty. Bitwise
 * or a background and foreground color together to get a complete color.
 */
#define TTY_FORE_BLACK		0x0
#define TTY_FORE_BLUE		0x1
#define TTY_FORE_GREEN		0x2
#define TTY_FORE_CYAN		0x3
#define TTY_FORE_RED		0x4
#define TTY_FORE_MAGENTA	0x5
#define TTY_FORE_BROWN		0x6
#define TTY_FORE_GREY		0x7

#define TTY_BACK_BLACK          (TTY_FORE_BLACK << 4)
#define TTY_BACK_BLUE           (TTY_FORE_BLUE << 4)
#define TTY_BACK_GREEN          (TTY_FORE_GREEN << 4)
#define TTY_BACK_CYAN           (TTY_FORE_CYAN << 4)
#define TTY_BACK_RED            (TTY_FORE_RED << 4)
#define TTY_BACK_MAGENTA        (TTY_FORE_MAGENTA << 4)
#define TTY_BACK_BROWN          (TTY_FORE_BROWN << 4)
#define TTY_BACK_GREY           (TTY_FORE_GREY << 4)

#include "ioctl.h"

/* tty structure. Contains all of the metadata for a tty. */
struct tty
{
	uint num; /* The number of this tty */
	uint active; /* 1: This tty is in the foreground, 0: background*/
	uchar type; /* The type of this tty. (see above.)*/
	char buffer[TTY_BUFFER_SZ]; /* Text buffer */
	uint cursor_pos; /* Text mode position of the cursor. */
	uint cursor_enabled; /* Whether or not to show the cursor (text)*/
	uint mem_start; /* The start of video memory. (color, mono only)*/
	uchar color; /* The current printing color of the terminal. */
	pid_t cpgid; /* The process group id of the controlling process. */
	uchar exclusive; /* Whether or not this tty can be opened */

	char keyboard[TTY_KEYBUFFER_SZ]; /* Keyboard input buffer */
	slock_t key_lock; /* The lock needed in order to read from keybaord */
	uint key_write; /* Write position in the buffer */
	uint key_read; /* Read position in the buffer */
	uint key_full; /* Is the buffer full? */
	uint key_nls; /* How many new lines are in the buffer? */

	struct DeviceDriver* driver; /* driver for standard in/out */

	/* Terminal operating settings */
	struct termios term;
	uchar termios_locked; /* 0 = unlocked, 1 = locked */

	/* Window size parameters */
	struct winsize window;
};

typedef struct tty* tty_t;

/**
 * Return a tty object type for the tty at the index. NULL is returned if
 * index is out of bounds.
 */
tty_t tty_find(uint index);

/**
 * Init a blank tty. The position of the cursor should start at 0. This
 * tty should start disabled. The text and graphic buffers should be
 * cleared. The default color is grey on black. The tty should start in
 * text mode.
 */
void tty_init(tty_t t, uint num, uchar type, uint cursor_enabled, 
	uint mem_start);

/**
 * Setup an io driver for a specific tty.
 */
int tty_io_setup(struct IODriver* driver, uint tty_num);

/**
 * Returns the number of this tty.
 */
uint tty_num(tty_t t);

/**
 * Enable this tty. This tty is now in the foreground and visible to the user.
 */
void tty_enable(tty_t t);

/**
 * Disable this tty. This tty is now in the background. Any data written to
 * this tty should be written to the buffer, but not written to memory.
 */
void tty_disable(tty_t t);

/** 
 * Print the character at the current cursor position.
 */
void tty_putc(tty_t t, char c);

/**
 * Print the formatted string at the current cursor position.
 */
void tty_printf(tty_t t, char* fmt, ...);

/**
 * Print the entire video memory to the screen (graphic mode only).
 */
void tty_print_screen(tty_t t, char* buffer);

/**
 * Get a character from the tty.
 */
char tty_getc(tty_t t);

/**
 * Sets whether or not the cursor should appear on the screen.
 */
uchar tty_set_cursor(tty_t t, uchar enabled);

/**
 * Sets the current position of the cursor for the specified mode.
 */
uchar tty_set_cursor_pos(tty_t t, uchar pos);

/**
 * Sets the current color of the tty output (text mode only).
 */
void tty_set_color(tty_t t, uchar color);

/**
 * Scroll down one line in the console.
 */
void tty_scroll(tty_t t);

/**
 * Handles the keyboard interrupt with the active tty.
 */
void tty_handle_keyboard_interrupt(void);

/**
 * Clear the input stream of the tty.
 */
void tty_clear_input(tty_t t);

#endif
