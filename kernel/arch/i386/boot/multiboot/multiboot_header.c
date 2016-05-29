#include "multiboot.h"

struct multiboot_header header = 
{
	MULTIBOOT_MAGIC,
	0,
	(uint32_t)-MULTIBOOT_MAGIC
};
