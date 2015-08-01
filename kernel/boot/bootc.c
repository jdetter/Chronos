#include "types.h"
#include "x86.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "elf.h"
#include "stdarg.h"
#include "serial.h"
#include "stdlib.h"
#include "fsman.h"
#include "ata.h"
#include "stdlib.h"
#include "stdmem.h"
#include "console.h"
#include "keyboard.h"
#include "vsfs.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

void setup_boot2_pgdir(void);
void cprintf(char* fmt, ...);
void __kernel_jmp__();
void __enable_paging__(uint* pgdir);

char* no_image = "Chronos.elf not found!\n";
char* invalid = "Chronos.elf is invalid!\n";
char* panic_str = "Boot strap is panicked.";

char* kernel_path = "/boot/chronos.elf";
char* checking_elf = "Chronos is ready to be loaded.\n";
char* kernel_loaded = "Chronos has been loaded.\n";
int serial = 0;

extern pgdir* k_pgdir;
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

	cprintf("Initilizing virtual memory...\t\t\t\t\t\t");
	setup_boot2_pgdir();
	cprintf("[ OK ]\n");

	cprintf("Initilizing paging...\t\t\t\t\t\t\t");
	__enable_paging__(k_pgdir);
	cprintf("[ OK ]\n");

	cprintf("Starting ata driver...\t\t\t\t\t\t\t");
	ata_init();
	cprintf("[ OK ]\n");

	cprintf("Kernel path: ");
	cprintf(kernel_path);
	cprintf("\n");

	struct vsfs_context context;
	cprintf("Starting VSFS driver...\t\t\t\t\t\t\t");
	uchar fake_cache[512];
	context.hdd = ata_drivers[0];
	if(vsfs_init(64, 0, 512, 512, fake_cache, &context)) panic("[FAIL]\n");
	cprintf("[ OK ]\n");

	vsfs_inode chronos_image;
	int inonum;
	if((inonum = vsfs_lookup(kernel_path, &chronos_image, &context)) == 0)
		panic(no_image);

	/* Sniff to see if it looks right. */
	uchar elf_buffer[512];
	vsfs_read(&chronos_image, elf_buffer, 0, 512, &context);
	char elf_buff[] = ELF_MAGIC;
	if(memcmp(elf_buffer, elf_buff, 4))
                panic(invalid);

	/* Lets try to load the kernel */
	uchar* kernel = (uchar*)KVM_KERN_S;

	/* Load the entire elf header. */
	struct elf32_header elf;
	vsfs_read(&chronos_image, &elf, 0, sizeof(struct elf32_header), &context);
	/* Check class */
	if(elf.exe_class != 1) panic("Bad class");	
	if(elf.version != 1) panic("Bad version");
	if(elf.e_type != ELF_E_TYPE_EXECUTABLE) panic("Bad type");	
	if(elf.e_machine != ELF_E_MACHINE_x86) panic("Bad ISA");	
	if(elf.e_version != 1) panic("Bad ISA version");	

	uint elf_entry = elf.e_entry;
	if(elf_entry != (uint)kernel) 
		panic("Wrong entry point: 0x%x", elf_entry);	

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
	
	/* Save vm context */
	save_vm();
	/* Jump into the kernel */
	__kernel_jmp__();

	panic("Jump failed.");
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
		/* Not going to bother with this */
	}

	va_end(&list);
}

void panic(char* fmt, ...)
{
        asm volatile("addl $0x08, %esp");
        asm volatile("call cprintf");
        for(;;);
}

void vm_stable_page_pool(void);
void setup_boot2_pgdir(void)
{
	k_pgdir = (pgdir*)KVM_KPGDIR;
	memset(k_pgdir, 0, PGSIZE);
	/* Do page pool */
	vm_stable_page_pool();
	vm_init_page_pool();
	
	/* Start by directly mapping the boot stage 2 pages */
	dir_mappages(KVM_BOOT2_S, KVM_BOOT2_E, k_pgdir, 0);

	/* Directly map the kernel page directory */
	dir_mappages(KVM_KPGDIR, KVM_KPGDIR + PGSIZE, k_pgdir, 0);

	/* Map null page temporarily */
	dir_mappages(0x0, PGSIZE, k_pgdir, 0);

	/* Map pages for the kernel binary */
	mappages(KVM_KERN_S, KVM_KERN_E - KVM_KERN_S, k_pgdir, 0);

	/* Directly map video memory */
	dir_mappages(CONSOLE_COLOR_BASE_ORIG, 
		CONSOLE_COLOR_BASE_ORIG + PGSIZE, 
		k_pgdir, 0);
	dir_mappages(CONSOLE_MONO_BASE_ORIG,
		CONSOLE_MONO_BASE_ORIG + PGSIZE,
		k_pgdir, 0);
}
