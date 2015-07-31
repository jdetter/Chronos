#ifndef _TYPES_H_
#define _TYPES_H_

#define FILE_MAX_PATH   256
#define FILE_MAX_NAME   256

/**
 * For use in functions requiring permissions:
 */

#define PERM_AAP 0777   /* Everyone has all permissions */
#define PERM_ARD 0444   /* Everyone can read */
#define PERM_AWR 0222   /* Everyone can write*/
#define PERM_AEX 0111   /* Everyone can execute */

#define PERM_UAP 0700   /* User has all permissions */
#define PERM_URD 0400   /* User can read */
#define PERM_UWR 0200   /* User can write */
#define PERM_UEX 0100   /* User can execute */

#define PERM_GAP 0070   /* Group has all permissions */
#define PERM_GRD 0040   /* Group can read */
#define PERM_GWR 0020   /* Group can write */
#define PERM_GEX 0010   /* Group can execute */

#define PERM_OAP 0007   /* Others have all permissions */
#define PERM_ORD 0004   /* Others can read */
#define PERM_OWR 0002   /* Others can write */
#define PERM_OEX 0001   /* Others can execute */

/* Unsigned types */
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

/* data type sizes for i386 */
typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef unsigned long uint_64;

/* Uncomment to enable global debug */
//#define __GLOBAL_DEBUG__

#ifdef __GLOBAL_DEBUG__
	static const uchar debug = 1;
#else
	static const uchar debug = 0;
#endif

#endif
