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
#include "ext2.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "diskio.h"
#include "diskcache.h"
#include "cacheman.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

void setup_boot2_pgdir(void);
void cprintf(char* fmt, ...);
void __kernel_jmp__(uint entry);
void __enable_paging__(uint* pgdir);

char* no_image = "Chronos.elf not found!\n";
char* invalid = "Chronos.elf is invalid!\n";
char* panic_str = "Boot strap is panicked.";

char* kernel_path = "/boot/chronos.elf";
char* kernel_loaded = "Chronos has been loaded.\n";
int serial = 0;
char* ok = "[ OK ]\n";
char* fail = "[FAIL]\n";

#define PG_SHIFT 12
#define EXT2_SUPER 2048 /* Where does the file system start? */
#define EXT2_SECT_SIZE 512
#define EXT2_INODE_CACHE_SZ 0x10000

extern pgdir* k_pgdir;
extern struct FSHardwareDriver* ata_drivers[];
struct FSDriver fs;

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
	cprintf(ok);

	cprintf("Initilizing paging...\t\t\t\t\t\t\t");
	__enable_paging__(k_pgdir);
	cprintf(ok);

	/* Initilize the cache manager */
	cman_init();

	cprintf("Starting ata driver...\t\t\t\t\t\t\t");
	ata_init();
	if(!ata_drivers[0]->valid)
	{
		cprintf(fail);
		panic("No disk drive attached!\n");
	}
	cprintf(ok);

	cprintf("Kernel path: ");
	cprintf(kernel_path);
	cprintf("\n");

	cprintf("Starting EXT2 driver...\t\t\t\t\t\t\t");
	diskio_setup(&fs);
	
	/* Initilize the file system driver */
	fs.valid = 1;
	fs.type = 1; /* EXT2 type here */
	fs.driver = ata_drivers[0];
	fs.start = EXT2_SUPER;
	
	/* Setup the cache */
	uint cache_sz = ATA_CACHE_SZ;
	void* disk_cache = cman_alloc(cache_sz);
	if(cache_init(disk_cache, cache_sz, PGSIZE,
		"EXT2 Disk", &ata_drivers[0]->cache))
	{
		panic(fail);
	}
	disk_cache_init(&fs);
	disk_cache_hardware_init(fs.driver);
	
	/* Start the driver */
	if(ext2_init(&fs))
		panic(fail);

	cprintf(ok);

	cprintf("Locating kernel...\t\t\t\t\t\t\t");
	void* chronos_inode;
	chronos_inode = fs.open(kernel_path, fs.context);
	if(!chronos_inode)
		panic(fail);
	cprintf(ok);

	/* Sniff to see if it looks right. */
	cprintf("Checking kernel sanity...\t\t\t\t\t\t");
	uchar elf_buffer[512];
	fs.read(chronos_inode, elf_buffer, 0, 512, fs.context);
	char elf_buff[] = ELF_MAGIC;
	if(memcmp(elf_buffer, elf_buff, 4))
                panic(fail);

	/* Load the entire elf header. */
	struct elf32_header elf;
	fs.read(chronos_inode, &elf, 0, sizeof(struct elf32_header), fs.context);
	/* Check class */
	if(elf.exe_class != 1) panic("Bad class");	
	if(elf.version != 1) panic("Bad version");
	if(elf.e_type != ELF_E_TYPE_EXECUTABLE) panic("Bad type");
	if(elf.e_machine != ELF_E_MACHINE_x86) panic("Bad ISA");
	if(elf.e_version != 1) panic("Bad ISA version");	

	/* Check size requirements */
	struct stat st;
	if(fs.stat(chronos_inode, &st, fs.context))
		panic(fail);

	if(st.st_size > KVM_KERN_E - KVM_KERN_S)
		panic(fail);

	uint elf_entry = elf.e_entry;
	cprintf(ok);

	int x;
	for(x = 0;x < elf.e_phnum;x++)
	{
		int header_loc = elf.e_phoff + (x * elf.e_phentsize);
		struct elf32_program_header curr_header;
		fs.read(chronos_inode, &curr_header, header_loc, 
			sizeof(struct elf32_program_header), 
			fs.context);
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
			uint needed_sz = PGROUNDUP(mem_sz);
			/* map some pages in for this space */
			mappages((uint)hd_addr, needed_sz, k_pgdir, 0);
			/* zero this region */
			memset(hd_addr, 0, mem_sz);
			/* Load the section */
			fs.read(chronos_inode, hd_addr, offset,
				file_sz, fs.context);
			/* By default, this section is rwx. */
		}
	}

	cprintf(kernel_loaded);
	
	/* Save vm context */
	save_vm();
	/* Jump into the kernel */
	__kernel_jmp__(elf_entry);

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
	// mappages(KVM_KERN_S, KVM_KERN_E - KVM_KERN_S, k_pgdir, 0);
	
	/* Map in some of the disk caching space */
	mappages(KVM_DISK_S, KVM_DISK_E - KVM_DISK_S, k_pgdir, 0);

	/* Directly map video memory */
	dir_mappages(CONSOLE_COLOR_BASE_ORIG, 
		CONSOLE_COLOR_BASE_ORIG + PGSIZE, 
		k_pgdir, 0);
	dir_mappages(CONSOLE_MONO_BASE_ORIG,
		CONSOLE_MONO_BASE_ORIG + PGSIZE,
		k_pgdir, 0);
}
