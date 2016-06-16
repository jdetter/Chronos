#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "file.h"
#include "stdlock.h"
#include "pipe.h"
#include "devman.h"
#include "tty.h"
#include "fsman.h"
#include "proc.h"
#include "chronos.h"

/** Check to see if an fd is valid */
int fd_ok(int fd)
{
        if(fd < 0 || fd >= PROC_MAX_FDS)
                return 0;
	if(!rproc->fdtab[fd])
		return 0;
        if(!rproc->fdtab[fd]->type)
                return 0;
        return 1;
}

/* Is the given address safe to access? */
int syscall_addr_safe(void* address)
{
	if(!address) return 1;
		/* Stack grows down towards heap*/
        if((rproc->stack_end <= (uint)address
                && (uint)address < rproc->stack_start) ||
		/* Code never changes, but end > start */
        	   (rproc->code_end > (uint)address
                && (uint)address >= rproc->code_start) ||
		/* MMAP grows down towards heap */
        	   (rproc->mmap_end <= (uint)address
                && (uint)address < rproc->mmap_start) ||
		/* HEAP grows upwards */
                   (rproc->heap_end > (uint)address
                && (uint)address >= rproc->heap_start))
        {
                /* Valid memory access */
                return 0;
        }
        /* Invalid memory access */
        return 1;
}

int syscall_ptr_safe(void* address)
{
        if(syscall_addr_safe(address)
                || syscall_addr_safe(address + 3))
                        return 1;
        return 0;
}

/**
 * Get an integer argument. The arg_num determines the offset to the argument.
 */
int syscall_get_int(int* dst, int arg_num)
{
	uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
        esp += arg_num;
        char* num_start = (char*)esp;
        char* num_end = (char*)esp + sizeof(int) - 1;

        if(syscall_addr_safe(num_start) || syscall_addr_safe(num_end))
                return 1;

        *dst = *esp;
        return 0;
}

/**
 * Get an integer argument. The arg_num determines the offset to the argument.
 */
int syscall_get_long(long* dst, int arg_num)
{
        uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
        esp += arg_num;
        char* num_start = (char*)esp;
        char* num_end = (char*)esp + sizeof(long) - 1;

        if(syscall_addr_safe(num_start) || syscall_addr_safe(num_end))
                return 1;

        *dst = *esp;
        return 0;
}

/**
 * Get an integer argument. The arg_num determines the offset to the argument.
 */
int syscall_get_short(short* dst, int arg_num)
{
        uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
        esp += arg_num;
        char* num_start = (char*)esp;
        char* num_end = (char*)esp + sizeof(short) - 1;

        if(syscall_addr_safe(num_start) || syscall_addr_safe(num_end))
                return 1;

        *dst = *esp;
        return 0;
}

char syscall_get_str(char* dst, int sz_kern, int arg_num)
{
	uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
        memset(dst, 0, sz_kern);
        esp += arg_num; /* This is a pointer to the string we need */
        char* str_addr = (char*)esp;
        if(syscall_ptr_safe(str_addr))
                return 1;
        char* str = *(char**)str_addr;

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

int syscall_get_str_ptr(const char** dst, int arg_num)
{
	uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
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

int syscall_get_buffer_ptr(char** ptr, int sz, int arg_num)
{
	uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
	esp += arg_num; /* This is a pointer to the string we need */
	char* buff_addr = (char*)esp;
	if(syscall_addr_safe(buff_addr))
		return 1;
	char* buff = *(char**)buff_addr;
	if(syscall_addr_safe(buff) || syscall_addr_safe(buff + sz - 1))
		return 1;

	*ptr = (char*)buff;

	return 0;
}

int syscall_get_buffer_ptrs(void*** ptr, int arg_num)
{
	uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
	esp += arg_num;
	/* Get the address of the buffer */
	char*** buff_buff_addr = (char***)esp;
	/* Is the user stack okay? */
	if(syscall_ptr_safe((char*) buff_buff_addr))
		return 1;
	char** buff_addr = *buff_buff_addr;
	/* Check until we hit a null. */
	int x;
	for(x = 0;x < MAX_ARG;x++)
	{
		if(syscall_ptr_safe((char*)(buff_addr + x)))
			return 1;
		char* val = *(buff_addr + x);
		if(val == 0) break;
	}

	if(x == MAX_ARG)
	{
		/* the list MUST end with a NULL element! */
		return 1;
	}

	/* Everything is reasonable. */
	*ptr = (void**)buff_addr;
	return 0;
}


int syscall_get_optional_ptr(void** ptr, int arg_num)
{
	uintptr_t* esp = (uintptr_t*)rproc->sys_esp;
	esp += arg_num;

	if(syscall_ptr_safe(esp))
		return 1;
	/* Get the pointer */
	*ptr = *(void**)esp;

	if(!ptr) return 1;

	return 0;
}
