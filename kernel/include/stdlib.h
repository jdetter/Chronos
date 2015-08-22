#ifndef _STDLIB_H_
#define _STDLIB_H_

/* Standard macro functions */
#define B4_ROUNDUP(addr)	((uint)addr + 3) & ~(3)
#define B4_ROUNDDOWN(addr)	((uint)addr) & ~(3)

/* Dependant headers */
#include "types.h"
#include "stdarg.h"

/**
 * Returns the length of the string. The null character doesn't count towards
 * the length of the string.
 */
uint strlen(const char* str);

/**
 * Change the case of all alphabetical ASCII characters in string str to their 
 * lower case representation.
 */
void tolower(char* str);

/**
 * Change the case of all alphabetical ASCII characters in string str to their 
 * upper case representation.
 */
void toupper(char* str);

/**
 * Copy the string src into the string dst with maximum number of sz bytes.
 * Returns the number of bytes that were copied. A null character MUST be the
 * last byte of the string.
 */
uint strncpy(char* dst, const char* src, uint sz);

/**
 * Copy str2 to the end of str1 with a total maximum of sz bytes. This function
 * MUST add a null character to the end that is within sz.
 */
uint strncat(char* str1, char* str2, uint sz);

/**
 * Compare str1 to str2. If str1 would come before str2 in the dictionary,
 * return -1. If str1 would come after str2 in the dictionary, return 1. If
 * the strings are equal, return 0. This function ignores case.
 */
int strcmp(const char* str1, const char* str2);

/**
 * Copy the memory from src to dst. If the two buffers overlap, this function
 * MUST account for that. The dst buffer must always be valid. A total of sz
 * bytes should be copied. 
 */
void memmove(void* dst, const void* src, uint sz);

/**
 * Fill the buffer dst with sz bytes that have the value val.
 */
void memset(void* dst, char val, uint sz);

/**
 * Compare the memory in buff1 to the memory in buff2 for a maximum of sz
 * bytes. If the buffers are equal, return 0. If the buffers are unequal,
 * return -1.
 */
int memcmp(void* buff1, void* buff2, uint sz);

/**
 * Convert a string to an int value. Parse until you hit a non numerical
 * character. Negative numbers are allowed.
 */
int atoi(char* str, int radix);

/**
 * Convert a string into a float value. Parse until you hit a non numerical
 * character. Negative numbers are allowed.
 */
float atof(char* str);

/**
 * Convert an integer to a string. This function will not modify more than sz
 * bytes of dst_c and will return a null terminated string.
 */
void itoa(int val_signed, char* dst_c, uint sz, uint radix);

/**
 * See printf function. This does the same thing except with a va_args list.
 */
int va_snprintf(char* dst, uint sz, va_list* list, char* fmt);

/**
 * Print the formatted string fmt into the buffer dst. It will print a
 * maximum of sz bytes into the buffer. This function is the exact same
 * thing as printf except it will print the characters into the buffer
 * instead of onto the screen.
 */
int snprintf(char* dst, uint sz, char* fmt, ...);

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
 * Convert a bcd coded byte into its binary equivalent.
 */
uchar bcdtobin(uchar val);

/**
 * Calculate the log base 2 of a number. Returns -1 if the
 * number is not a power of 2 (there is a remainder).
 */
int log2(uint val);

#endif
