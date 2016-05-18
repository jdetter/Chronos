#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include <stdint.h>

#define MULTIBOOT_MAGIC 0x1BADB002

struct multiboot_header
{
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
};

/* multiboot_header.flags[16] must be 1 */
struct multiboot_header_extended1
{
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;

	uint32_t header_addr;
	uint32_t load_addr;
	uint32_t load_end_addr;
	uint32_t bss_end_addr;
	uint32_t entry_addr;
};

/* multiboot_header.flags[2] must be 1 */
struct multiboot_header_extended2
{
        uint32_t magic;
        uint32_t flags;
        uint32_t checksum;

        uint32_t header_addr;
        uint32_t load_addr;
        uint32_t load_end_addr;
        uint32_t bss_end_addr;
        uint32_t entry_addr;

	uint32_t mode_type;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
};

extern struct multiboot_header header;
extern void multiboot_main(void);

#endif
