#ifndef _STDIO_H_
#define _STDIO_H_

/* Dependant headers */
#include "types.h"

#define STDIN 		0x00
#define STDOUT 		0x01
#define STDERR 		0x02

/**
 * Print to the console using the format string fmt.
 */
int printf(char* fmt, ...);

/**
 * Get a character from the user.
 */
char getc(void);

/**
 * Get a string from the file descriptor fd. If a new line character is
 * encountered, parsing will stop and the function will return. The function
 * returns the amount of characters read from the console.
 */
int fgets(char* dst, uint sz, uint fd);

/**
 * Print the string str to the console with a new line character at the end.
 */
int puts(char* src);

#endif
