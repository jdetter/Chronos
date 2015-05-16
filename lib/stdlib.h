#ifndef _STDLIB_H_
#define _STDLIB_H_

/**
 * Returns the length of the string. The null character doesn't count towards
 * the length of the string.
 */
uint strlen(char* str);

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
uint strncpy(char* dst, char* src, uint sz);

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
int strcmp(char* str1, char* str2);

/**
 * Copy the memory from src to dst. If the two buffers overlap, this function
 * MUST account for that. The dst buffer must always be valid. A total of sz
 * bytes should be copied. 
 */
void memmove(void* dst, void* src, uint sz);

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
 * Print the formatted string fmt into the buffer dst with maximum bytes sz.
 * The string dst must end with a null terminating character.
 */
int snprintf(char* dst, uint sz, char* fmt, ...);

/**
 * Allocate sz bytes on the heap. You should return a pointer to the allocated 
 * region. For performance reasons, the allocated block should be 16 byte 
 * aligned.
 */
void* malloc(uint sz);

/**
 * Free the block that was eariler allocated by malloc. The free list should 
 * be coalesced to try to make the largest free blocks possible. Returns
 * whether or not the block was freed.
 */
int mfree(void* ptr);

/**
 * minit should setup the global variables needed for this allocator to 
 * function. It should call mmap and request an amount of memory. This memory 
 * amount should be defined using #define MEM_AMT so that it can easily be 
 * changed for tuning.
 */
void minit(void);

#endif
