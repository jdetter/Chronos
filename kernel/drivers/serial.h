#ifndef _SERIAL_H_
#define _SERIAL_H_

/**
 * Initilize the serial ports. If pic is enabled, it will modify
 * the pic mask so that the pic gets serial interrupts.
 */
int serial_init(int pic);

/**
 * Write sz bytes in buffer dst to the serial port. Returns the amount
 * of bytes written to the port. If there was any error, 0 will be returned.
 */
uint serial_write(void* dst, uint sz);

/**
 * Read at most sz bytes from the serial port. Returns the amount of bytes
 * read from the buffer.
 */
uint serial_read(void* dst, uint sz);

#endif
