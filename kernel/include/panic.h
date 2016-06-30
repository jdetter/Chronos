#ifndef _PANIC_H_
#define _PANIC_H_

/**
 * Initilize the primary tty for the first time.
 */
void cprintf_init(void);

/**
 * Hacky version of printf that doesn't require va_args. This method
 * is dangerous if not called properly. This method can be called any time
 * once the kernel has been loaded.
 */
void cprintf(char* fmt, ...);
void panic(char* fmt, ...) __attribute__ ((noreturn));
#endif


/* 
 * TODO: At some point this should be it's own header and automagically imported during the build
 * process, for now we'll keep it in panic since this is imported everywhere we're expected to
 * print.
 * */
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "kconfig.h"
#include <stdbool.h>

/*
 * This header file is used to define macros for cprintk for use when debugging.
 *
 * As a user to enable debug prints define CONFIG_DEBUG when compiling. 
 * (gcc -DCONFIG_DEBUG)
 *
 * To enable debugging ouput for a certain verbosity and below define D_LEVEL as 
 * one of the following (in increasing level of verbosity): (gcc -DD_LEVEL=<LEVEL>)
 *
 * NONE, INFO, DEBUG, SPEW, ALL, DEFAULT
 *
 *
 * To enable debugging output for a specific system define D_SYSTEM as one of 
 * the following:
 *
 * NONE, GLOBAL, KERNEL, DRIVER, OTHER, DEFAULT
 *
 *
 * For both of these, if none is defined DEFAULT is chosen.
 *
 * TODO: Enable support for multiple D_SYSTEMs. 
 *
 * The best way to do this would be to add a config system and have that define all the 
 * systems we want. For now if you need to print multiple systems, (but don't want GLOBAL), 
 * you'll need to manually | DEBUG_SYSTEM with whatever D_SYSTEM_s you want to debug.
 * */

#define DEVEL(level, fmt...)        //TODO: Print at level

#define DEVEL_LEVEL(level)      //TODO: Define the level at PREVIOUS_DEVEL_LEVEL << 1

/* Debug verbosity, higher number is more verbose! */
#define _D_LEVEL_PREFIX									D_LEVEL_
#define	D_LEVEL_NONE									0
#define	D_LEVEL_INFO									1
#define D_LEVEL_DEBUG									2
#define	D_LEVEL_SPEW									3
#define D_LEVEL_ALL										10

#define D_LEVEL_DEFAULT									D_LEVEL_ALL

/* This should be defined in the makefile or config */
#ifndef	D_LEVEL
#define D_LEVEL											DEFAULT
#endif

/* Actually set the level to print debug at using macro expansion.*/
#define _D_CONCAT_(a, b)								a ## b
#define _D_CONCAT(a, b)									_D_CONCAT_(a, b)
#define DEBUG_LEVEL										_D_CONCAT(_D_LEVEL_PREFIX, D_LEVEL)

#define _D_SYSTEM_PREFIX								D_SYSTEM_
#define D_SYSTEM_NONE									0
#define D_SYSTEM_GLOBAL									0x1 << 0
#define D_SYSTEM_KERNEL									0x1 << 1
#define D_SYSTEM_DRIVER									0x1 << 2
#define D_SYSTEM_OTHER									0x1 << 3
#define D_SYSTEM_DEFAULT								D_SYSTEM_GLOBAL

/* This should be defined in the makefile or config. */
#ifndef D_SYSTEM
#define D_SYSTEM										DEFAULT
#endif

#define DEBUG_SYSTEM									_D_CONCAT(_D_SYSTEM_PREFIX, D_SYSTEM)


#if IS_ENABLED(CONFIG_DEBUG)
#define debug_cprintf(fmt...) cprintf(fmt)
#else
#define debug_cprintf(...)
#endif

#define DEBUG_THIS_LEVEL(verbosity)												\
		DEBUG_LEVEL >= verbosity		?				true:false


#define DEBUG_THIS_SYSTEM(system)												\
		(system & DEBUG_SYSTEM)			?				true:false


#if IS_ENABLED(CONFIG_DEBUG)
/*TODO: If there is ever a need to add subsystem debugging, add it to this define since there should
 *		be no need to use this yet.. I'd recommend adding subsytem between system and fmt.*/
#define SYSTEM_DEBUG(verbosity, system, fmt...)											\
		(																				\
				{																		\
						if(DEBUG_THIS_LEVEL(verbosity) && DEBUG_THIS_SYSTEM(system))	\
		debug_cprintf(fmt);																\
				}																		\
		)
#else
#define SYSTEM_DEBUG(...)
#endif

#if IS_ENABLED(CONFIG_DEBUG)
#define GLOBAL_DEBUG(verbosity, system, fmt...)									\
        SYSTEM_DEBUG(verbosity, (system | D_SYSTEM_GLOBAL), fmt)

#define DEBUG(verbosity, system, args...)										\
        GLOBAL_DEBUG(verbosity, system, args)
#else 
#define DEBUG(...)
#endif

#endif
