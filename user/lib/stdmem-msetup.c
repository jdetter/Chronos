#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "stdmem.h"
#include "chronos.h"

void msetup(void)
{
	uint start_addr = (uint)mmap(NULL, M_AMT, PROT_READ | PROT_WRITE);
	uint end_addr = start_addr + M_AMT;
	minit(start_addr, end_addr);
}

