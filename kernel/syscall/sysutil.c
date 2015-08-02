#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "stdlib.h"
#include "pipe.h"
#include "devman.h"
#include "tty.h"
#include "fsman.h"
#include "proc.h"
#include "chronos.h"

extern struct proc* rproc;

/* Is the given address safe to access? */
uchar syscall_addr_safe(void* address)
{
        if((rproc->stack_end >= (uint)address
                && (uint)address < rproc->stack_start) ||
        	   (rproc->mmap_end >= (uint)address
                && (uint)address < rproc->mmap_start) ||
                   (rproc->heap_end < (uint)address
                && (uint)address >= rproc->heap_start))
        {
                /* Valid memory access */
                return 0;
        }
        /* Invalid memory access */
        return 1;
}

uchar syscall_ptr_safe(void* address)
{
        if(syscall_addr_safe(address)
                || syscall_addr_safe(address + 3))
                        return 1;
        return 0;
}

/**
 * Get an integer argument. The arg_num determines the offset to the argument.
 */
uchar syscall_get_int(int* dst, uint arg_num)
{
	uint* esp = rproc->sys_esp;
        esp += arg_num;
        uchar* num_start = (uchar*)esp;
        uchar* num_end = (uchar*)esp + sizeof(int) - 1;

        if(syscall_addr_safe(num_start) || syscall_addr_safe(num_end))
                return 1;

        *dst = *esp;
        return 0;
}

/**
 * Get an integer argument. The arg_num determines the offset to the argument.
 */
uchar syscall_get_long(long* dst, uint arg_num)
{
        uint* esp = rproc->sys_esp;
        esp += arg_num;
        uchar* num_start = (uchar*)esp;
        uchar* num_end = (uchar*)esp + sizeof(long) - 1;

        if(syscall_addr_safe(num_start) || syscall_addr_safe(num_end))
                return 1;

        *dst = *esp;
        return 0;
}

/**
 * Get a string argument. The arg_num determines the offset to the argument.
 */
uchar syscall_get_buffer(char* dst, uint sz_user, uint sz_kern, uint arg_num)
{
	uint* esp = rproc->sys_esp;
        memset(dst, 0, sz_kern);
        esp += arg_num; /* This is a pointer to the string we need */
        uchar* str_addr = (uchar*)esp;
        if(syscall_ptr_safe(str_addr))
                return 1;
        uchar* str = *(uchar**)str_addr;
        if(syscall_addr_safe(str) || syscall_addr_safe(str + sz_user - 1))
                return 1;

        uint sz = sz_user;
        if(sz_kern < sz_user)
                sz = sz_kern;
        int x;
        for(x = 0;x < sz;x++)
        {
                if(str[x] == 0)
                {
                        dst[x] = 0;
                        break;
                }

                dst[x] = str[x];
        }

        return 0;
}

uchar syscall_get_str(char* dst, uint sz_kern, uint arg_num)
{
	uint* esp = rproc->sys_esp;
        memset(dst, 0, sz_kern);
        esp += arg_num; /* This is a pointer to the string we need */
        uchar* str_addr = (uchar*)esp;
        if(syscall_ptr_safe(str_addr))
                return 1;
        uchar* str = *(uchar**)str_addr;

        int x;
        for(x = 0;x < sz_kern;x++)
        {
                if(syscall_ptr_safe(str + x))
			return 1;
		dst[x] = str[x];
		if(str[x] == 0) break;
	}

	return 0;
}

uchar syscall_get_str_ptr(const char** dst, uint arg_num)
{
	uint* esp = rproc->sys_esp;
	esp += arg_num; /* This is a pointer to the string we need */
	const char* str_addr = (const char*)esp;
	if(syscall_ptr_safe((void*)str_addr))
		return 1;
	const char* str = *(const char**)str_addr;

	int x;
	for(x = 0;;x++)
	{
		if(syscall_addr_safe((void*)(str + x)))
			return 1;
		if(!str[x]) break;
	}

	*dst = str;
	return 0;
}

uchar syscall_get_buffer_ptr(char** ptr, uint sz, uint arg_num)
{
	uint* esp = rproc->sys_esp;
	esp += arg_num; /* This is a pointer to the string we need */
	uchar* buff_addr = (uchar*)esp;
	if(syscall_addr_safe(buff_addr))
		return 1;
	uchar* buff = *(uchar**)buff_addr;
	if(syscall_addr_safe(buff) || syscall_addr_safe(buff + sz - 1))
		return 1;

	*ptr = (char*)buff;

	return 0;
}

uchar syscall_get_buffer_ptrs(uchar*** ptr, uint arg_num)
{
	uint* esp = rproc->sys_esp;
	esp += arg_num;
	/* Get the address of the buffer */
	uchar*** buff_buff_addr = (uchar***)esp;
	/* Is the user stack okay? */
	if(syscall_ptr_safe((uchar*) buff_buff_addr))
		return 1;
	uchar** buff_addr = *buff_buff_addr;
	/* Check until we hit a null. */
	int x;
	for(x = 0;x < MAX_ARG;x++)
	{
		if(syscall_ptr_safe((uchar*)(buff_addr + x)))
			return 1;
		uchar* val = *(buff_addr + x);
		if(val == 0) break;
	}

	if(x == MAX_ARG)
	{
		/* the list MUST end with a NULL element! */
		return 1;
	}

	/* Everything is reasonable. */
	*ptr = buff_addr;
	return 0;
}

