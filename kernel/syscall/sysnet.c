#include "types.h"
#include "stdlock.h"
#include "file.h"
#include "syscall.h"
#include "devman.h"
#include "fsman.h"
#include "netman.h"
#include "syscall.h"
#include "stdlib.h"
#include "chronos.h"
#include "pipe.h"
#include "tty.h"
#include "proc.h"
#include "panic.h"

extern struct proc* rproc;

int sys_gethostname(void)
{
	char* buffer;
	size_t size;
	
	if(syscall_get_int((int*)&size, 1)) return 0;
	if(syscall_get_buffer_ptr((void**)&buffer, size, 0)) return 0;

	strncpy(buffer, hostname, size);
	
	return 0;
}
