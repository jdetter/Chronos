#ifndef _TERMIOS_H_
#define _TERMIOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/ioctl.h>

/**
 * Gets the parameters associated with the object referred
 * to by fd and stores them in the termios structure referenced
 * by termios_p. This function may be invoked from a background
 * process; however, the terminal attributes may be changed by
 * a foreground process. Returns 0 on success, -1 otherwise.
 */
int tcgetattr(int fd, struct termios* termios_p);

/**
 * Sets the parameters associated with the terminal from the
 * termios structure referenced by termios_p. Optional actions
 * determines when the action takes effect. Returns 0 on success,
 * -1 otherwise.
 */
int tcsetattr(int fd, int optional_actions, 
		const struct termios* termios_p);

/**
 * Send a break to the terminal. This has no effect in chronos even
 * if the terminal is operating over a serial connection.
 */
int tcsendbreak(int fd, int duration);

/**
 * Waits until all output written to the object referred to by
 * fd has been transmitted.
 */
int tcdrain(int fd);

/**
 * discard data written to the object referred to by fd but not
 * transmitted, or data received but not read, depending on
 * the queue selector.
 */
int tcflush(int fd, int queue_selector);

/**
 * Suspend transmission and reception of data on the object
 * referred to by fd depending on the value of action.
 */
int tcflow(int fd, int action);

/**
 * Put the terminal into raw mode. Input is available on a character
 * by character basis. Echo is disabled. All special processing
 * of terminal input and output is disabled.
 */
void cfmakeraw(struct termios* termios_p);

/**
 * Returns the input baud rate into the termios_p structure.
 */
speed_t cfgetispeed(const struct termios* termios_p);

/**
 * Returns the output baud rate stored in the termios
 * structure pointed to by termios_p.
 */
speed_t cfgetospeed(const struct termios* termios_p);

/**
 * Sets the input baud rate in termios_p. Look for the speed
 * constants in ioctl.h.
 */
int cfsetispeed(struct termios* termios_p, speed_t speed);

/**
 * Set the output baud rate in termios_p. Look for the speed
 * constants in ioctl.h
 */
int cfsetospeed(struct termios* termios_p, speed_t speed);

/**
 * Sets both the input and output baud rates.
 */
int cfsetspeed(struct termios* termios_p, speed_t speed);

#ifdef __cplusplus
}
#endif

#endif
