#include "types.h"
#include "elf.h"
#include "serial.h"
#include "stdlib.h"
#include "ata.h"
#include "stdlib.h"
#include "stdmem.h"
#include "console.h"
#include "keyboard.h"
#include "vsfs.h"

#define KERNEL_LOAD 0x100000

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

void panic();

char* welcome = "Welcome to Chronos!\n";
char* no_image = "Chronos.elf not found!\n";
char* invalid = "Chronos.elf is invalid!\n";
char* panic_str = "Boot strap is panicked.";

char* kernel_search = "Kernel path: ";
char* kernel_path = "/boot/chronos.elf";

extern int b_off;

int main(void)
{
	int serial = 1;
	if(serial_init(0))
	{
		/* There is no serial port. */
		serial = 0;
	}
	if(serial) serial_write((char*)welcome, strlen(welcome));

	/* Initilize memory allocator. */
	minit(0x60A00, 0x70000, 0);

	cinit();
	cprintf(welcome);

	/* Lets find the kernel */
	vsfs_init(64);

	serial_write(kernel_search, strlen(kernel_search));
	cprintf(kernel_search);
	serial_write(kernel_path, strlen(kernel_path));
	cprintf(kernel_path);
	serial_write("\n", 1);
	cprintf("\n");

	vsfs_inode chronos_image;
	int inonum;
	if((inonum = vsfs_lookup(kernel_path, &chronos_image)) == 0)
	{
		serial_write(no_image, strlen(no_image));
		cprintf(no_image);
		panic();
	}

	cprintf("\n");
	/* Sniff to see if it looks right. */
	uchar elf_buffer[512];
	vsfs_read(&chronos_image, 0, 512, elf_buffer);
	char elf_buff[] = ELF_MAGIC;
	if(memcmp(elf_buffer, elf_buff, 4))
	{
		serial_write(invalid, strlen(invalid));
                cprintf(invalid);
                panic();
	}	
	

	/* Lets try to load the kernel */
	//uchar* kernel = (uchar*)KERNEL_LOAD;


	panic();
	return 0;
}

void panic()
{
        serial_write(panic_str, strlen(panic_str)); 
        cprintf(panic_str); 
        for(;;);
}
