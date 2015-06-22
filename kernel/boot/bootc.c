#include "types.h"
#include "elf.h"
#include "stdarg.h"
#include "serial.h"
#include "stdlib.h"
#include "file.h"
#include "fsman.h"
#include "ata.h"
#include "stdlib.h"
#include "stdmem.h"
#include "console.h"
#include "keyboard.h"
#include "stdlock.h"
#include "vsfs.h"

#define KERNEL_LOAD 0x100000

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

void cprintf(char* fmt, ...);
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
int serial = 0;

extern struct FSHardwareDriver* ata_drivers[];

int main(void)
{
	serial = 1;
	if(serial_init(0))
	{
		/* There is no serial port. */
		serial = 0;
	}
	cprintf("Welcome to Chronos!\n");

	cprintf("Starting memory allocator...\t\t\t");
	/* Initilize memory allocator. */
	minit(0x60A00, 0x70000);
	cprintf("[ OK ]\n");

	cprintf("Starting ata driver...\t\t\t\t");
	ata_init();
	cprintf("[ OK ]\n");

	cprintf(kernel_search);
	cprintf(kernel_path);
	cprintf("\n");

	struct vsfs_context context;
	cprintf("Starting VSFS driver...\t\t\t\t");
	uchar fake_cache[512];
	context.hdd = ata_drivers[0];
	if(vsfs_init(64, 0, 512, 512, fake_cache, &context))
	{
		cprintf("[FAIL]\n");
		panic();
	}
	cprintf("[ OK ]\n");

	vsfs_inode chronos_image;
	int inonum;
	if((inonum = vsfs_lookup(kernel_path, &chronos_image, &context)) == 0)
	{
		cprintf(no_image);
		panic();
	}

	/* Sniff to see if it looks right. */
	uchar elf_buffer[512];
	vsfs_read(&chronos_image, elf_buffer, 0, 512, &context);
	char elf_buff[] = ELF_MAGIC;
	if(memcmp(elf_buffer, elf_buff, 4))
	{
		cprintf(invalid);
                panic();
	}	

	/* Lets try to load the kernel */
	uchar* kernel = (uchar*)KERNEL_LOAD;

	/* Load the entire elf header. */
	struct elf32_header elf;
	vsfs_read(&chronos_image, &elf, 0, sizeof(struct elf32_header), &context);
	/* Check class */
	if(elf.exe_class != 1) panic();	
	if(elf.version != 1) panic();
	if(elf.e_type != ELF_E_TYPE_EXECUTABLE) panic();	
	if(elf.e_machine != ELF_E_MACHINE_x86) panic();	
	if(elf.e_version != 1) panic();	

	uint elf_entry = elf.e_entry;
	if(elf_entry != (uint)kernel) panic();	

	cprintf(checking_elf);

	int x;
	for(x = 0;x < elf.e_phnum;x++)
	{
		int header_loc = elf.e_phoff + (x * elf.e_phentsize);
		struct elf32_program_header curr_header;
		vsfs_read(&chronos_image, &curr_header, header_loc, 
			sizeof(struct elf32_program_header), 
			&context);
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
			vsfs_read(&chronos_image, hd_addr, offset,
				file_sz, &context);
			/* By default, this section is rwx. */
		}
	}

	cprintf(kernel_loaded);

	/* Jump into the kernel */
	__kernel_jmp__();

	panic();
	return 0;
}



void cprintf(char* fmt, ...)
{
	char buffer[128];
	va_list list;
	va_start(&list, (void**)&fmt);
	va_snprintf(buffer, 128, &list, fmt);
	if(serial)
	{
		serial_write(buffer, strlen(buffer));
	} else {
		/* Figure this out later*/
	}

	va_end(&list);
}

void panic()
{
        serial_write(panic_str, strlen(panic_str)); 
        for(;;);
}

char* msetup_err = "Error: memory allocator not initlized.";
void msetup()
{
	serial_write(msetup_err, strlen(msetup_err));
	panic();
}

