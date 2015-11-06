#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <panic.h>
#include <vm.h>

#include "kern/types.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "elf.h"


int check_binary_path(const char* path)
{
	inode ino = fs_open(path, O_RDONLY, 0644, 0, 0);
	if(!ino) return -1;
	uchar result = check_binary_inode(ino);
	fs_close(ino);
	return result;
}

uintptr_t load_binary_path(const char* path, pgdir* pgdir, uintptr_t* start, 
	uintptr_t* end, int user)
{
        inode ino = fs_open(path, O_RDONLY, 0644, 0x0, 0x0);
        if(!ino) return 0;
        uintptr_t result = load_binary_inode(ino, pgdir, start, end, user);
        fs_close(ino);
        return result;
}

int check_binary_inode(inode ino)
{
	if(!ino) return -1;

        /* Sniff to see if it looks right. */
        uchar elf_buffer[4];
        fs_read(ino, elf_buffer, 4, 0);
        char elf_buff[] = ELF_MAGIC;
        if(memcmp(elf_buffer, elf_buff, 4)) return -1;

        /* Load the entire elf header. */
        struct elf32_header elf;
        fs_read(ino, &elf, sizeof(struct elf32_header), 0);

        /* Check class */
        if(elf.exe_class != 1) return -1;
        if(elf.version != 1) return -1;
        if(elf.e_type != ELF_E_TYPE_EXECUTABLE) return -1;
        if(elf.e_machine != ELF_E_MACHINE_x86) return -1;
        if(elf.e_version != 1) return -1;

	return 0;
}

uintptr_t load_binary_inode(inode ino, pgdir* pgdir, uintptr_t* seg_start, 
		uintptr_t* seg_end, int user)
{
	if(!ino) return 0;

	if(check_binary_inode(ino))
		return 0;

	/* Load the entire elf header. */
        struct elf32_header elf;
        fs_read(ino, &elf, sizeof(struct elf32_header), 0);

	uint elf_end = 0;
        uint elf_entry = elf.e_entry;

	uint code_start = (int)-1;
	uint code_end = 0;
	int x;
        for(x = 0;x < elf.e_phnum;x++)
        {
                int header_loc = elf.e_phoff + (x * elf.e_phentsize);
                struct elf32_program_header curr_header;
                fs_read(ino, &curr_header,
                        sizeof(struct elf32_program_header),
                        header_loc);
                /* Skip null program headers */
                if(curr_header.type == ELF_PH_TYPE_NULL) 
			continue;

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
			/* Paging: allocate user pages */
			
			pgflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
			pgflags_t tbl_flags = 0;
			
			if(user) 
				dir_flags |= VM_DIR_USRP;
			else dir_flags |= VM_DIR_KRNP;

                        /* Should this section be readable? */
                        if(curr_header.flags & ELF_PH_FLAG_R)
                                tbl_flags |= VM_TBL_READ;

			/* Should this section be executable? */
			if(curr_header.flags & ELF_PH_FLAG_X)
                                tbl_flags |= VM_TBL_EXEC;

			/* Map the pages into memory */
			if(vm_mappages(hd_addr, mem_sz, pgdir, 
					dir_flags, tbl_flags))
				return 0;

			if(hd_addr + mem_sz > elf_end)
				elf_end = hd_addr + mem_sz;

                        /* zero this region */
                        memset((void*)hd_addr, 0, mem_sz);

			/* Is this a new start? */
			if((uint)hd_addr < code_start)
				code_start = (uint)hd_addr;

			/* Is this a new end? */
			if((uint)(hd_addr + mem_sz) > code_end)
				code_end = (uint)(hd_addr + mem_sz);

                        /* Load the section */
                        if(fs_read(ino, (void*)hd_addr, file_sz, offset) 
					!= file_sz)
				return 0;

			/* Should we make these pages writable? */
			if(!(curr_header.flags & ELF_PH_FLAG_W))
			{
				if(vm_pgsreadonly((uintptr_t)hd_addr, 
						(uintptr_t)hd_addr 
						+ mem_sz, pgdir))
					return 0;

			}
                }
        }

	/* Set the entry point of the program */
	if(seg_start)
		*seg_start = code_start;
	if(seg_end)
		*seg_end = code_end;

	return elf_entry;
}

