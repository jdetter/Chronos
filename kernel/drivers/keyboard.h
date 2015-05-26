#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

/**
 * Initilize the keyboard controller
 */
int kbd_init();

/**
 * Get an ASCII character from the keyboard if possible. If an ASCII character
 * is not ready, 0 will be returned.
 */
char kbd_getc();

#endif
