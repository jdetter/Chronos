#ifndef _KLOG_H_
#define _KLOG_H_

#define NKLOG 32

/* Dependant headers */
#include <stdint.h>
#include "fsman.h"

struct klog
{
	int valid; /* Whether or not this struct is in use. */
	inode i;
	off_t offset;
};

typedef struct klog* klog_t;

/**
 * Initilize the klogging structures.
 */
void klog_init(void);

/**
 * Allocate a new klogging structure. Return the new klog. Returns NULL if
 * there are no more klog structures left. Append is whether or not to
 * append to the end of the log. The path is the path to the log file to 
 * use.
 */
klog_t klog_alloc(int append, char* path);

/**
 * Free the given klog structure. The file will also be closed.
 */
void klog_free(klog_t log);

/**
 * Write to the log file. Return the amount of bytes written to the file.
 */
int klog_write(klog_t log, void* buffer, size_t sz);

#endif
