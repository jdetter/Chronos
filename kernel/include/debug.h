#ifndef _DEBUG_H_
#define _DEBUG_H_

//#include <stdarg.h>
#include "kconfig.h"
#include <stdbool.h>
//#include "debug_level.h"

/* Debug verbosity, higher number is more verbose! */
#define _D_LEVEL_PREFIX									D_LEVEL_
#define	D_LEVEL_NONE									0
#define	D_LEVEL_INFO									1
#define D_LEVEL_DEBUG									2
#define	D_LEVEL_SPEW									3
#define D_LEVEL_ALL										10

#define D_LEVEL_DEFAULT									D_LEVEL_ALL

/* This should be defined in the makefile or config */
#ifndef	LOG_LEVEL
#define LOG_LEVEL										NONE
#endif

/* Actually set the level to print debug at using macro expansion.*/
#define _D_CONCAT_(a, b)								a ## b
#define _D_CONCAT(a, b)									_D_CONCAT_(a, b)
#define DEBUG_LEVEL										_D_CONCAT(_D_LEVEL_PREFIX, D_LEVEL)

#define _D_SYSTEM_PREFIX								D_SYSTEM_
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
		DEBUG_LEVEL >= verbosity	? true:false


#define DEBUG_THIS_SYSTEM(system)												\
		(system | DEBUG_SYSTEM) ? true:false


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
