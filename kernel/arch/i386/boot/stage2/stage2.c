#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "kstdlib.h"
#include "x86.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "elf.h"
#include "drivers/serial.h"
#include "fsman.h"
#include "drivers/ata.h"
#include "stdlib.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/ext2.h"
#include "proc.h"
#include "vm.h"
#include "storageio.h"
#include "storagecache.h"
#include "cacheman.h"
#include "panic.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

static void setup_boot2_pgdir(void);
void __kernel_jmp__(uintptr_t entry);

static char* kernel_path = "/boot/chronos.elf";
static int serial = 0;

static char* ok = "[ OK ]\n";
static char* fail = "[FAIL]\n";

#define PG_SHIFT 12
#define EXT2_SUPER 2048 /* Where does the file system start? */
#define EXT2_SECT_SIZE 512
#define EXT2_INODE_CACHE_SZ 0x10000

extern pgdir_t* k_pgdir;
extern struct StorageDevice* ata_drivers[];
static struct FSDriver fs;

int main(void)
{
	/* Check to see if the serial port is attached */
	serial = 1;
	if(serial_init(0))
	{
		/* There is no serial port. */
		serial = 0;
	}

	cprintf("Initilizing virtual memory...\t\t\t\t\t\t");

	/* Setup our stage 2 page table */
	setup_boot2_pgdir();

	/* Enable paging */
	vm_enable_paging(k_pgdir);

	cprintf(ok);
	cprintf("Welcome to Chronos!\n");

	/* Kernel can write to readonly pages, disable this */
	cprintf("Enforcing kernel readonly...\t\t\t\t\t\t");
	vm_enforce_kernel_readonly();
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

	/* Initilize the file system driver */
	fs.valid = 1;
	fs.type = 1; /* EXT2 type here */
	fs.driver = ata_drivers[0];
	fs.fs_start = EXT2_SUPER;

	/* Setup the cache */
	size_t cache_sz = ATA_CACHE_SZ;
	void* disk_cache = cman_alloc(cache_sz);
	if(cache_init(disk_cache, cache_sz, PGSIZE,
				"EXT2 Disk", &ata_drivers[0]->cache))
	{
		panic(fail);
	}
	storage_cache_init(&fs);
	storage_cache_hardware_init(fs.driver);

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
	char elf_buffer[512];
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

	uintptr_t elf_entry = elf.e_entry;
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
			uintptr_t hd_addr = (uintptr_t)curr_header.virt_addr;
			off_t offset = curr_header.offset;
			size_t file_sz = curr_header.file_sz;
			size_t mem_sz = curr_header.mem_sz;
			size_t needed_sz = PGROUNDUP(mem_sz);

			vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
			vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT;

			/* map some pages in for this space */
			vm_mappages(hd_addr, needed_sz, k_pgdir, 
					dir_flags, tbl_flags);

			/* zero this region */
			memset((void*)hd_addr, 0, mem_sz);
			/* Load the section */
			fs.read(chronos_inode, (void*)hd_addr, offset,
					file_sz, fs.context);

			/* Security: check for read only */
			if(!(curr_header.flags & ELF_PH_FLAG_W))
				if(vm_pgsreadonly(hd_addr, hd_addr + needed_sz,
							k_pgdir))
					panic("bootstrap: readonly failed!\n");
		}
	}

	cprintf("Chronos has been loaded.\n");

	/* Save vm state */
	vm_alloc_save_state();
	/* Jump into the kernel */
	__kernel_jmp__(elf_entry);

	panic("Jump failed.");
	return 0;
}


void cprintf(char* fmt, ...)
{
	char buffer[128];
	va_list list;
	va_start(list, fmt);
	vsnprintf(buffer, 128, fmt, list);
	if(serial)
	{
		serial_write(buffer, strlen(buffer));
	} else {
		/* Not going to bother with this */
	}

	va_end(list);
}

void panic(char* fmt, ...)
{
	asm volatile("addl $0x08, %esp");
	asm volatile("call cprintf");
	for(;;);
}

static void setup_boot2_pgdir(void)
{
	/** 
	 * When this function completes, the page pool should be mapped into
	 * the new page directory. The boot code, data and stack should also
	 * be mapped along with the e820 table.
	 */

	/* Initilize our page directory */
	k_pgdir = (pgdir_t*)KVM_KPGDIR;
	/* No entries */
	memset(k_pgdir, 0, PGSIZE);

	/* Add all free pages to the page pool */
	vm_init_page_pool();

	/* Default flags for kernel page directory entries */
	vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT;

	/* Start by directly mapping the boot stage 2 executable (data + code) */
	vm_dir_mappages(BOOT2_S, BOOT2_E, k_pgdir, dir_flags, tbl_flags);

	/* Directly map the current stack */
	// vm_dir_mappages(BOOT2_STACK_BOT, BOOT2_STACK_TOP, k_pgdir, 
			// dir_flags, tbl_flags);

	/* Directly map the kernel page directory */
	vm_dir_mappages(KVM_KPGDIR, KVM_KPGDIR + PGSIZE, k_pgdir, 
			dir_flags, tbl_flags);

	/* Directly map the e820 table (kernel will cleanup when booted) */
	// vm_dir_mappages(BOOT2_E820_S, BOOT2_E820_E, k_pgdir, 
			// dir_flags, tbl_flags);

	/* Map null page temporarily */
	vm_dir_mappages(0x0, PGSIZE, k_pgdir, dir_flags, tbl_flags);

	/* Map in the disk caching space */
	if(vm_mappages(KVM_DISK_S, KVM_DISK_E - KVM_DISK_S, k_pgdir, 
			dir_flags, tbl_flags))
		panic("\nFAILURE: System doesn't have enought available RAM.\n");

	/* Don't cache and enable write through for the hardware mappings */
	tbl_flags |= VM_TBL_WRTH | VM_TBL_CACH;

	/* Directly map video memory */
	vm_dir_mappages(CONSOLE_COLOR_BASE_ORIG, CONSOLE_COLOR_BASE_ORIG + PGSIZE, 
			k_pgdir, dir_flags, tbl_flags);
	vm_dir_mappages(CONSOLE_MONO_BASE_ORIG, CONSOLE_MONO_BASE_ORIG + PGSIZE,
			k_pgdir, dir_flags, tbl_flags);
}
