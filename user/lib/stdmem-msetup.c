#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "stdmem.h"
#include "chronos.h"

void msetup(void)
{
	uint start_addr = (uint)mmap(NULL, M_AMT, PROT_READ | PROT_WRITE,
		0x0, 0, 0);
	uint end_addr = start_addr + M_AMT;
	minit(start_addr, end_addr);
}

int mpanic(int size)
{
	int pages = (size + 4095) / 4096;
	uint start_addr = (uint)mmap(NULL, pages, PROT_READ | PROT_WRITE,
		0x0, 0, 0);
	if(start_addr == 0) return 1; /* We couldn't expand. */
        uint end_addr = start_addr + pages;
        mexpand(start_addr, end_addr);
	return 0; /* Expansion successful. */
}
