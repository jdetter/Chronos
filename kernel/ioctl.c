#include <stdint.h>
#include <stdlib.h>
#include "syscall.h"

/**
 * ioctl helper function
 */
int ioctl_arg_ok(void* addr, size_t sz)
{
	if(syscall_addr_safe(addr) || syscall_addr_safe(addr + sz - 1))
                return 1;
	return 0;
}
