/*
 * Copyright (C) 2016 coreboot project https://www.coreboot.org/
 *
 * coreboot is licensed under the terms of the GNU General Public License (GPL).
 * Some files are licensed under the "GPL (version 2, or any later version)",
 * and some files are licensed under the "GPL, version 2". For some parts, which
 * were derived from other projects, other (GPL-compatible) licenses may apply.
 * Please check the individual source files for details.
 */
#ifndef __KCONFIG_H__
#define __KCONFIG_H__

/*
 * Getting something that works in C and CPP for an arg that may or may
 * not be defined is tricky.  Here, if we have "#define CONFIG_BOOGER 1"
 * we match on the placeholder define, insert the "0," for arg1 and generate
 * the triplet (0, 1, 0).  Then the last step cherry picks the 2nd arg (a one).
 * When CONFIG_BOOGER is not defined, we generate a (... 1, 0) pair, and when
 * the last step cherry picks the 2nd arg, we get a zero.
 */
#define __ARG_PLACEHOLDER_1 0,
#define config_enabled(cfg) _config_enabled(cfg)
#define _config_enabled(value) __config_enabled(__ARG_PLACEHOLDER_##value)
#define __config_enabled(arg1_or_junk) ___config_enabled(arg1_or_junk 1, 0, 0)
#define ___config_enabled(__ignored, val, ...) val

#define IS_ENABLED(option) config_enabled(option)

#endif
