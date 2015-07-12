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
/* Types that conflict with the Linux definitions*/

typedef unsigned int off_t; /* Used to represent file offsets */
typedef unsigned int size_t; /* Used to represent the size of an object. */
typedef signed int uid_t; /* User id*/
typedef signed int gid_t; /* Group id*/

/* File permissions */
typedef unsigned int mode_t;

typedef signed int pid_t; /* Process ID */
typedef signed int clock_t;

/* NULL */
#define NULL ((void*)0)

/* Some POSIX definitions: */
typedef unsigned int dev_t;
typedef unsigned int ino_t;
typedef unsigned int nlink_t;
typedef unsigned int blksize_t;
typedef unsigned int blkcnt_t;
typedef signed int time_t;
typedef signed int suseconds_t;

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
