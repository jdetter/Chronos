#include "kern/types.h"
#include "syscall.h"

/**
 * ioctl helper function
 */
uchar ioctl_arg_ok(void* addr, uint sz)
{
	if(syscall_addr_safe(addr) || syscall_addr_safe(addr + sz - 1))
                return 1;
	return 0;
}
