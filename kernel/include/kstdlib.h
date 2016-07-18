#ifndef _KERNEL_STDLIB_H_
#define _KERNEL_STDLIB_H_

/* Dependant headers */
#include <stdint.h>
#include <stdarg.h>

/* Standard macro functions */
#define B4_ROUNDUP(addr)	((uintptr_t)addr + 3) & ~(3)
#define B4_ROUNDDOWN(addr)	((uintptr_t)addr) & ~(3)


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
void kitoa(int val_signed, char* dst_c, size_t sz, int radix);

/**
 * Calculate the log base 2 of a number. Returns -1 if the
 * number is not a power of 2 (there is a remainder).
 */
int __log2(int val);

/**
 * Convert a bcd coded byte into its binary equivalent.
 */
char bcdtobin(char val);



#endif
