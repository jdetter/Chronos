#ifndef _ELF_H_
#define _ELF_H_

#include <stdint.h>
#include "stdlock.h"
#include "fsman.h"
#include "vm.h"

#define ELF_MAGIC {0x7F, 'E', 'L', 'F'}
#define ELF_MAGIC_INT 0x464C457F

/* os_abi options*/
#define ELF_OS_ABI_SYSTEM_V 	0x00
#define ELF_OS_ABI_HP_UX 	0x01
#define ELF_OS_ABI_NET_BSD 	0x02
#define ELF_OS_ABI_LINUX 	0x03
#define ELF_OS_ABI_SOLARIS 	0x06
#define ELF_OS_ABI_AIX 		0x07
#define ELF_OS_ABI_IRIX 	0x08
#define ELF_OS_ABI_FREE_BSD 	0x09
#define ELF_OS_ABI_OPEN_BSD 	0x0C
#define ELF_OS_ABI_OPEN_VMS 	0x0D

/* e_type options */
#define ELF_E_TYPE_RELOCATE 	0x1
#define ELF_E_TYPE_EXECUTABLE 	0x2
#define ELF_E_TYPE_SHARED 	0x3
#define ELF_E_TYPE_CORE 	0x4

/* e_machine options */
#define ELF_E_MACHINE_SPARC   	0x02
#define ELF_E_MACHINE_x86     	0x03
#define ELF_E_MACHINE_MIPS    	0x08
#define ELF_E_MACHINE_POWER   	0x14
#define ELF_E_MACHINE_ARM     	0x28
#define ELF_E_MACHINE_SUPER  	0x2A
#define ELF_E_MACHINE_IA_64   	0x32
#define ELF_E_MACHINE_x86_64  	0x3E
#define ELF_E_MACHINE_AARCH64 	0xB7

struct elf32_header
{
	uint32_t  magic; /* This should be equal to ELF_MAGIC */
	uint8_t exe_class; /* 1 = 32bit, 2 = 64bit*/
	uint8_t endianess; /* 1 = little, 2 = big */
	uint8_t version; /* Set to 1 */
	uint8_t os_abi; /* This is often 0, reguardless of the target platform.*/
	uint32_t padding_low_abi; /* Further defines abi */
	uint32_t padding_high;
	uint16_t e_type; /* See e_type definitions above */	
	uint16_t e_machine; /* See e_machine definitions above */	
	uint32_t e_version; /* Set to 1 */	
	uint32_t e_entry; /* Entry point where the program should start */	
	uint32_t e_phoff; /* Points to the start of the program header table */	
	uint32_t e_shoff; /* Points to the start of the section header table */
	uint32_t e_flags; /* Architecture dependant */
	uint16_t e_ehsize; /* The size in bytes of this header */
	uint16_t e_phentsize; /* The size of a program header table entry */
	uint16_t e_phnum; /* Number of program headers */
	uint16_t e_shentsize; /* The size of a section header table entry */
	uint16_t e_shnum; /* Contains the number of sections  */
	uint16_t e_shstrndx; /* pointer to section names */
};

/* program header type options */
#define ELF_PH_TYPE_NULL	0x00
#define ELF_PH_TYPE_LOAD	0x01
#define ELF_PH_TYPE_DYNAMIC	0x02
#define ELF_PH_TYPE_INTERP	0x03
#define ELF_PH_TYPE_NOTE	0x04
#define ELF_PH_TYPE_SHLIB	0x05
#define ELF_PH_TYPE_PHDR	0x06
#define ELF_PH_TYPE_LOPROC	0x70000000
#define ELF_PH_TYPE_HIPROC	0x7fffffff
#define ELF_PH_TYPE_GNU_STACK	0x6474e551

/* Flags for the program header */
#define ELF_PH_FLAG_X		0x1
#define ELF_PH_FLAG_W		0x2
#define ELF_PH_FLAG_R		0x4

struct elf32_program_header
{
	uint32_t type; /* The type of this program header */
	uint32_t offset; /* The offset in the file where this segment starts */
	uint32_t virt_addr; /* The virtual address where this segments is loaded */
	uint32_t phy_addr; /* Physical addressing is not relevant. */
	uint32_t file_sz; /* The size of this segment in the file */
	uint32_t mem_sz; /* The size of this segment in memory. */
	uint32_t flags; /* Read, Write Execute. See above. */
	uint32_t align; /* The memory alignment requrired by this segment. */
};

/**
 * Check the binary denoted by the given path. Returns 0 if the binary 
 * looks ok to load.
 */
int elf_check_binary_inode(inode ino);

/**
 * Check the binary denoted by the given path. Returns 0 if the binary 
 * looks ok to load. DEPREICATED USE check_binary_inode.
 */
int elf_check_binary_path(const char* path);

/**
 * Load the binary into memory denoted by the given inode. The start of the 
 * code segment (low) will be placed into start if it is not NULL. The end of
 * the code segment (high) is returned in end if it is not null. Returns 0
 * on success, non zero otherwise.
 */
uintptr_t elf_load_binary_inode(inode ino, pgdir_t* pgdir, uintptr_t* start, 
	uintptr_t* end, int user);

/**
 * Load the binary into memory denoted by the given path. The start of the 
 * code segment (low) will be placed into start if it is not NULL. The end of
 * the code segment (high) is returned in end if it is not null. Returns 0
 * on success, non zero otherwise. DEPRICATED USE load_binary_inode.
 */
uintptr_t elf_load_binary_path(const char* path, pgdir_t* pgdir, 
	uintptr_t* start, uintptr_t* end, int user);

#endif
