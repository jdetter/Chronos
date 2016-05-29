#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define PGSIZE 4096
#include "kern/types.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "drivers/lwfs.h"


struct FSDriver driver;

int main(int argc, char** argv)
{
	uintptr_t area_sz = 0x1000000;
	void* area = mmap(NULL, area_sz, PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	lwfs_init((uintptr_t)area >> 12, 
		((uintptr_t)area >> 12) + (area_sz >> 12),
		PGSIZE, 0, NULL, &driver, driver.context);

	char test_buffer[1234567];
	memset(test_buffer, 0, 1234567);
	driver.create("/test.txt", 0644, 0, 0, driver.context);
	void* i = driver.open("/test.txt", driver.context);
	driver.write(i, test_buffer, 0, 1234567, driver.context);

	return 0;
}
