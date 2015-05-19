#ifndef _ELF_H_
#define _ELF_H_

#define ELF_MAGIC {0x7F, 'E', 'L', 'F'}

/* e_type options */
#define E_TYPE_RELOCATE 	0x1
#define E_TYPE_EXECUTABLE 	0x2
#define E_TYPE_SHARED 		0x3
#define E_TYPE_CORE 		0x4

/* e_machine options */
#define E_MACHINE_SPARC   	0x02
#define E_MACHINE_x86     	0x03
#define E_MACHINE_MIPS    	0x08
#define E_MACHINE_POWER   	0x14
#define E_MACHINE_ARM     	0x28
#define E_MACHINE_SUPER  	0x2A
#define E_MACHINE_IA_64   	0x32
#define E_MACHINE_x86_64  	0x3E
#define E_MACHINE_AARCH64 	0xB7

struct elf32_header
{
	uint_32  magic; /* This should be equal to ELF_MAGIC */
	uint_8 exe_class; /* 1 = 32bit, 2 = 64bit*/
	uint_8 endianess; /* 1 = little, 2 = big */
	uint_8 version; /* Set to 1 */
	uint_8 os_abi; /* This is often 0, reguardless of the target platform.*/
	uint_8 abi_version; /* Further defines abi */
	uint_32  padding_low4;
	uint_8 padding_5;
	uint_8 padding_6;
	uint_8 padding_7;
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

struct elf32_program_header
{
	uint_32 type; /* */
};

#endif
