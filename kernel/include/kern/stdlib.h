#include "kern/types.h"

#ifndef _KERN_STDLIB_H_
#define _KERN_STDLIB_H_

/* Standard macro functions */
#define B4_ROUNDUP(addr)	((uint)addr + 3) & ~(3)
#define B4_ROUNDDOWN(addr)	((uint)addr) & ~(3)

/* Dependant headers */
#include "stdarg.h"

/**
 * Trim white space from the beginning and end of the string. Returns the
 * resulting length of the string.
 */
int trim(char* str);

/**
 * Returns 1 if the character is printable, 0 otherwise.
 */
int ascii_char(char c);

/**
 * Convert an integer to a string. This function will not modify more than sz
 * bytes of dst_c and will return a null terminated string.
 */
void kitoa(int val_signed, char* dst_c, uint sz, uint radix);

/**
 * Calculate the log base 2 of a number. Returns -1 if the
 * number is not a power of 2 (there is a remainder).
 */
int __log2(uint val);

/**
 * Convert a bcd coded byte into its binary equivalent.
 */
uchar bcdtobin(uchar val);

#endif
