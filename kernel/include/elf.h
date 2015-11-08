#ifndef _ELF_H_
#define _ELF_H_

#define ELF_MAGIC {0x7F, 'E', 'L', 'F'}

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
	uint_32  magic; /* This should be equal to ELF_MAGIC */
	uint_8 exe_class; /* 1 = 32bit, 2 = 64bit*/
	uint_8 endianess; /* 1 = little, 2 = big */
	uint_8 version; /* Set to 1 */
	uint_8 os_abi; /* This is often 0, reguardless of the target platform.*/
	uint_32 padding_low_abi; /* Further defines abi */
	uint_32  padding_high;
	uint_16 e_type; /* See e_type definitions above */	
	uint_16 e_machine; /* See e_machine definitions above */	
	uint_32 e_version; /* Set to 1 */	
	uint_32 e_entry; /* Entry point where the program should start */	
	uint_32 e_phoff; /* Points to the start of the program header table */	
	uint_32 e_shoff; /* Points to the start of the section header table */
	uint_32 e_flags; /* Architecture dependant */
	uint_16 e_ehsize; /* The size in bytes of this header */
	uint_16 e_phentsize; /* The size of a program header table entry */
	uint_16 e_phnum; /* Number of program headers */
	uint_16 e_shentsize; /* The size of a section header table entry */
	uint_16 e_shnum; /* Contains the number of sections  */
	uint_16 e_shstrndx; /* pointer to section names */
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
	uint_32 type; /* The type of this program header */
	uint_32 offset; /* The offset in the file where this segment starts */
	uint_32 virt_addr; /* The virtual address where this segments is loaded */
	uint_32 phy_addr; /* Physical addressing is not relevant. */
	uint_32 file_sz; /* The size of this segment in the file */
	uint_32 mem_sz; /* The size of this segment in memory. */
	uint_32 flags; /* Read, Write Execute. See above. */
	uint_32 align; /* The memory alignment requrired by this segment. */
};

#include "stdlock.h"
#include "fsman.h"
#include "vm.h"

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
uintptr_t elf_load_binary_inode(inode ino, pgdir* pgdir, uintptr_t* start, 
	uintptr_t* end, int user);

/**
 * Load the binary into memory denoted by the given path. The start of the 
 * code segment (low) will be placed into start if it is not NULL. The end of
 * the code segment (high) is returned in end if it is not null. Returns 0
 * on success, non zero otherwise. DEPRICATED USE load_binary_inode.
 */
uintptr_t elf_load_binary_path(const char* path, pgdir* pgdir, 
	uintptr_t* start, uintptr_t* end, int user);

#endif
