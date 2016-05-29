#ifndef _DISKIO_H_
#define _DISKIO_H_

#include "fsman.h"

/**
 * Enable the hardware driver to use the read and write functions.
 */
void diskio_setup(struct FSDriver* driver);

#endif
