#ifndef _TYPES_H_
#define _TYPES_H_

/* Unsigned types */
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

/* data type sizes for i386 */
typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef unsigned long uint_64;

#ifndef __LINUX_DEFS__
typedef unsigned long off_t; /* Used to represent file offsets */
typedef unsigned long size_t; /* Used to represent the size of an object. */

/* NULL */
#define NULL ((void*)0)

#endif

typedef uint pgdir;
typedef uint pgtbl;

/* Uncomment to enable global debug */
//#define __GLOBAL_DEBUG__

#ifdef __GLOBAL_DEBUG__
	static const uchar debug = 1;
#else
	static const uchar debug = 0;
#endif

#endif
