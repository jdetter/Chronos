#ifndef _IOSCHED_H_
#define _IOSCHED_H_

/**
 * IO Scheduler
 */

/**
 * Block for reading from the keyboard. When this function
 * returns, the dst buffer will have a maximum of request_size
 * bytes in it.
 */
int block_keyboard_io(void* dst, int request_size);

/**
 * Signal a tty for keyboard input.
 */
void signal_keyboard_io(tty_t t);

/**
 * Do a round of io scheduling.
 */
void iosched_check(void);

#endif
