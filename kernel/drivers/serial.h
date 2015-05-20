#ifndef _SERIAL_H_
#define _SERIAL_H_

#define SERIAL_1 0x0
#define SERIAL_2 0x1
#define SERIAL_3 0x2
#define SERIAL_4 0x4

/**
 * Initilize the serial ports.
 */
void serial_init();

/**
 * Write sz bytes in buffer dst to the serial port. Returns the amount
 * of bytes written to the port. If there was any error, 0 will be returned.
 */
uint serial_write(void* dst, uint sz, uint port);

/**
 * Read at most sz bytes from the serial port. Returns the amount of bytes
 * read from the buffer.
 */
uint serial_read(void* dst, uint sz, uint port);

#endif
