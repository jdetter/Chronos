#include "types.h"
#include "elf.h"
#include "stdarg.h"
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

void __kernel_jmp__();

void panic();

char* welcome = "Welcome to Chronos!\n";
char* no_image = "Chronos.elf not found!\n";
char* invalid = "Chronos.elf is invalid!\n";
char* panic_str = "Boot strap is panicked.";

char* kernel_search = "Kernel path: ";
char* kernel_path = "/boot/chronos.elf";
char* checking_elf = "Chronos is ready to be loaded.\n";
char* kernel_loaded = "Chronos has been loaded.\n";

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

	console_init();
	//cprintf(welcome);

	/* Lets find the kernel */
	vsfs_init(64);

	serial_write(kernel_search, strlen(kernel_search));
	//cprintf(kernel_search);
	serial_write(kernel_path, strlen(kernel_path));
	//cprintf(kernel_path);
	serial_write("\n", 1);
	//cprintf("\n");

	vsfs_inode chronos_image;
	int inonum;
	if((inonum = vsfs_lookup(kernel_path, &chronos_image)) == 0)
	{
		serial_write(no_image, strlen(no_image));
		//cprintf(no_image);
		panic();
	}

	/* Sniff to see if it looks right. */
	uchar elf_buffer[512];
	vsfs_read(&chronos_image, 0, 512, elf_buffer);
	char elf_buff[] = ELF_MAGIC;
	if(memcmp(elf_buffer, elf_buff, 4))
	{
		serial_write(invalid, strlen(invalid));
                //cprintf(invalid);
                panic();
	}	

	/* Lets try to load the kernel */
	uchar* kernel = (uchar*)KERNEL_LOAD;

	/* Load the entire elf header. */
	struct elf32_header elf;
	vsfs_read(&chronos_image, 0, sizeof(struct elf32_header), &elf);
	/* Check class */
	if(elf.exe_class != 1) panic();	
	if(elf.version != 1) panic();
	if(elf.e_type != ELF_E_TYPE_EXECUTABLE) panic();	
	if(elf.e_machine != ELF_E_MACHINE_x86) panic();	
	if(elf.e_version != 1) panic();	

	uint elf_entry = elf.e_entry;
	if(elf_entry != (uint)kernel) panic();	

	//cprintf(checking_elf);
	serial_write(checking_elf, strlen(checking_elf));

	int x;
	for(x = 0;x < elf.e_phnum;x++)
	{
		int header_loc = elf.e_phoff + (x * elf.e_phentsize);
		struct elf32_program_header curr_header;
		vsfs_read(&chronos_image, header_loc, 
			sizeof(struct elf32_program_header), 
			&curr_header);
		/* Skip null program headers */
		if(curr_header.type == ELF_PH_TYPE_NULL) continue;
		
		/* 
		 * GNU Stack is a recommendation by the compiler
		 * to allow executable stacks. This section doesn't
		 * need to be loaded into memory because it's just
		 * a flag.
		 */
		if(curr_header.type == ELF_PH_TYPE_GNU_STACK)
			continue;
	
		if(curr_header.type == ELF_PH_TYPE_LOAD)
		{
			/* Load this header into memory. */
			uchar* hd_addr = (uchar*)curr_header.virt_addr;
			uint offset = curr_header.offset;
			uint file_sz = curr_header.file_sz;
			uint mem_sz = curr_header.mem_sz;
			/* zero this region */
			memset(hd_addr, 0, mem_sz);
			/* Load the section */
			vsfs_read(&chronos_image, offset, 
				file_sz, hd_addr);
			/* By default, this section is rwx. */
		}
	}

	//cprintf(kernel_loaded);
	serial_write(kernel_loaded, strlen(kernel_loaded));

	/* Jump into the kernel */
	__kernel_jmp__();

	panic();
	return 0;
}

void panic()
{
        serial_write(panic_str, strlen(panic_str)); 
        //cprintf(panic_str); 
        for(;;);
}

/* mmap function to get stdlib to compile */
void* mmap(void* hint, uint sz, int protection)
{panic();return NULL;}
