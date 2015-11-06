/**
 * Author: John Detter <john@detter.com>
 *
 * Page directory functions.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "kern/stdlib.h"
#include "x86.h"
#include "file.h"
#include "stdarg.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "cpu.h"

/** Default kernel directory and table flags for i386 */
#define DFT_KDIRFLAGS (VM_DIR_PRES | VM_DIR_READ | VM_DIR_WRIT | VM_DIR_KRNP)
#define DFT_KTBLFLAGS (VM_TAB_PRES | VM_TAB_READ | VM_TAB_KRNP)

/** Default user directory and table flags for i386 */
#define DFT_UDIRFLAGS (VM_DIR_PRES | VM_DIR_READ | VM_DIR_WRIT | VM_DIR_USRP)
#define DFT_UTBLFLAGS (VM_TAB_PRES | VM_TAB_READ | VM_TAB_USRP)

uchar __kvm_stack_check__(void);
void __kvm_swap__(pgdir* kvm);
uchar __check_paging__(void);

extern pgdir* k_pgdir;
void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user, uint flags);
void vm_safe_mapping(uint start, uint end, pgdir* dir);

void vm_clear_uvm_kstack(pgdir* dir);
void vm_set_uvm_kstack(pgdir* dir, pgdir* swap);

/* PGDIR FLAG CONVERSIONS */
pgflags_t vm_dir_flags(pgflags_t flags)
{
	return 0;
}

pgflags_t vm_tbl_flags(pgflags_t flags)
{
        pgflags_t result = 0;
        if(flags & VM_TAB_GLBL)
		result |= 
        if(flags & VM_TAB_DRTY)

        if(flags & VM_TAB_ACSS)

        if(flags & VM_TAB_CACH)

        if(flags & VM_TAB_USRP)

        if(flags & VM_TAB_WRIT)

        if(flags & VM_TAB_PRES)
}


pgdir* vm_push_pgdir(void)
{
	/* Is paging even enabled? */
	if(!__check_paging__()) return NULL;
	pgdir* dir = vm_curr_pgdir();
	if(dir == k_pgdir) return NULL;
	push_cli();
	/* Are we on the kernel stack? */
	if(!__kvm_stack_check__())
		__kvm_swap__(k_pgdir);
	else vm_enable_paging(k_pgdir);
	return dir;	
}

void vm_pop_pgdir(pgdir* dir)
{
	if(dir == NULL) return;
	if(!__check_paging__()) return;
        if(dir == k_pgdir) return;
	//vm_clear_uvm_kstack(k_pgdir);
	vm_enable_paging(dir);
	pop_cli();
}

int vm_mappage(uintptr_t phy, uintptr_t virt, pgdir* dir, pgflags_t flags)
{
        pgdir* save = vm_push_pgdir();
         dir_flags = KDIRFLAGS;
        uint tbl_flags = KTBLFLAGS;

        if(user)
        {
                dir_flags = UDIRFLAGS;
                tbl_flags = UTBLFLAGS;
        }

        phy = PGROUNDDOWN(phy);
        virt = PGROUNDDOWN(virt);
        uint dir_index = PGDIRINDEX(virt);
        /* Do we need to allocate a new page table? */
        if(!dir[dir_index]) dir[dir_index] = palloc() | dir_flags;

        uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
        uint tbl_index = PGTBLINDEX(virt);
        if(!tbl[tbl_index])
        {
                tbl[tbl_index] = phy | tbl_flags | flags;
        } else {
                panic("kvm remap: 0x%x\n", virt);
        }
        vm_pop_pgdir(save);

        return 0;
}

int vm_mappages(uintptr_t va, size_t sz, pgdir* dir, uchar user)
{
	pgdir* save = vm_push_pgdir();
        /* round va + sz up to a page */
        uintptr_t end = PGROUNDDOWN((va + sz + PGSIZE - 1));
        uintptr_t start = PGROUNDDOWN(va);
        if(debug)
                cprintf("Creating virtual mapping from 0x%x to 0x%x\n",
                                start, end);
        if(end <= start) 
	{
		vm_pop_pgdir(save);
		return -1;
	}
        uintptr_t x;
        for(x = start;x != end;x += PGSIZE)
        {
                uintptr_t pg = vm_findpg(x, 1, dir, user);
                if(debug) cprintf("Virtual page 0x%x mapped to 0x%x\n", x, pg);
        }

	vm_pop_pgdir(save);

	return 0;
}

int vm_dir_mappages(uintptr_t start, uintptr_t end, pgdir* dir, uchar user)
{
	pgdir* save = vm_push_pgdir();
        start = PGROUNDDOWN(start);
        end = PGROUNDUP(end);
        uint x;
        for(x = start;x < end;x += PGSIZE)
                if(vm_mappage(x, x, dir, user, 0))
			return -1;
	vm_pop_pgdir(save);

	return 0;
}

uintptr_t vm_unmappage(uintptr_t virt, pgdir* dir)
{
        pgdir* save = vm_push_pgdir();
        virt = PGROUNDDOWN(virt);
        uint dir_index = PGDIRINDEX(virt);
        uint tbl_index = PGTBLINDEX(virt);

        if(!dir[dir_index])
        {
                vm_pop_pgdir(save);
                return 0;
        }

        uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
        if(tbl[tbl_index])
        {
                uint page = PGROUNDDOWN(tbl[tbl_index]);
                tbl[tbl_index] = 0;
                vm_pop_pgdir(save);
                return page;
        }

        vm_pop_pgdir(save);
        return 0;
}

void vm_safe_mapping(uint start, uint end, pgdir* dir)
{
	pgdir* save = vm_push_pgdir();
        start = PGROUNDDOWN(start);
        end = PGROUNDUP(end);
        uint page;
        for(page = start;page < end;page += PGSIZE)
        {
                /* Security: do not directly map guard pages */
                if(page == KVM_KSTACK_G1 || page == KVM_KSTACK_G2)
                        continue;
                if(!vm_findpg(page, 0, k_pgdir, 0))
                        vm_mappage(page, page, k_pgdir, 0, 0);
        }
	vm_pop_pgdir(save);
}

uintptr_t vm_findpg(uintptr_t virt, int create, pgdir* dir, uchar user)
{
	pgdir* save = vm_push_pgdir();
        uint dir_flags = KDIRFLAGS;
        uint tbl_flags = KTBLFLAGS;

        if(user)
        {
                dir_flags = UDIRFLAGS;
                tbl_flags = UTBLFLAGS;
        }

        virt = PGROUNDDOWN(virt);
        uint dir_index = PGDIRINDEX(virt);
        uint tbl_index = PGTBLINDEX(virt);

        if(!dir[dir_index])
        {
                if(create) dir[dir_index] = palloc() | dir_flags;
                else 
		{
			vm_pop_pgdir(save);
			return 0;
		}
        }

        uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
        uint page;
        if(!(page = tbl[tbl_index]))
        {
                if(create) page = tbl[tbl_index] = palloc() | tbl_flags;
                else 
		{
			vm_pop_pgdir(save);
			return 0;
		}
        }

	vm_pop_pgdir(save);
        return PGROUNDDOWN(page);
}

int findpgflags(uint virt, pgdir* dir)
{
        pgdir* save = vm_push_pgdir();

        virt = PGROUNDDOWN(virt);
	uint dir_index = PGDIRINDEX(virt);
	uint tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return -1;
	}

	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	uint page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return -1;
	}

	vm_pop_pgdir(save);
	return page & (PGSIZE - 1);
}

uint pgflags(uint virt, pgdir* dir, uchar user, uchar flags)
{
	pgdir* save = vm_push_pgdir();
	uint tbl_flags = KTBLFLAGS;

	if(user) tbl_flags = UTBLFLAGS;

	virt = PGROUNDDOWN(virt);
	uint dir_index = PGDIRINDEX(virt);
	uint tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return 0;
	}

	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	uint page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return 0;
	} else {
		page = PGROUNDDOWN(page);
		page |= tbl_flags | flags;
		tbl[tbl_index] = page;
	}

	vm_pop_pgdir(save);
	return PGROUNDDOWN(page);
}

uint pgreadonly(uint virt, pgdir* dir, uchar user)
{
	pgdir* save = vm_push_pgdir();
	uint tbl_flags = KTBLFLAGS;

	if(user) tbl_flags = UTBLFLAGS;

	virt = PGROUNDDOWN(virt);
	uint dir_index = PGDIRINDEX(virt);
	uint tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return 0;
	}

	uint* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
	uint page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return 0;
	} else {
		tbl_flags &= ~(PGTBL_RW);
		page = PGROUNDDOWN(page);
		page |= tbl_flags;
		tbl[tbl_index] = page;
	}

	vm_pop_pgdir(save);
	return PGROUNDDOWN(page);
}

uint pgsreadonly(uint virt_start, uint virt_end, pgdir* dir, uchar user)
{
	virt_start = PGROUNDDOWN(virt_start);
	virt_end = PGROUNDUP(virt_end);

	uint addr = virt_start;
	for(;addr < virt_end;addr += PGSIZE)
		if(pgreadonly(addr, dir, user) == 0)
			return -1;

	return 0;
}

uint vm_memmove(void* dst, const void* src, uint sz, 
		pgdir* dst_pgdir, pgdir* src_pgdir)
{
	if(sz == 0) return 0;
	pgdir* save = vm_push_pgdir();
	uint bytes = 0; /* bytes copied */

	while(bytes != sz)
	{
		/* How many bytes are there left to copy? */
		uint left = sz - bytes;
		int to_copy = left;

		/* The source page */
		uint src_page = vm_findpg((uint)src + bytes, 0, src_pgdir, 1);
		uint src_offset = ((uint)src + bytes) & (PGSIZE - 1);
		uint src_cpy = PGSIZE - src_offset;
		if(!src_page) break;

		/* Check destination page */
		uint dst_page = vm_findpg((uint)dst + bytes, 1, dst_pgdir, 1);
		uint dst_offset = ((uint)dst + bytes) & (PGSIZE - 1);
		uint dst_cpy = PGSIZE - dst_offset;
		if(!dst_page) break;

		/* Make sure copy amount is okay */
		if(src_cpy < to_copy) to_copy = src_cpy;
		if(dst_cpy < to_copy) to_copy = dst_cpy;
		memmove((void*)dst_page + dst_offset, 
				(void*)src_page + src_offset, to_copy);
		/* Adjust bytes */
		bytes += to_copy;
	}

	vm_pop_pgdir(save);
	return bytes;
}

void vm_copy_kvm(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	uint x;
	for(x = UVM_KVM_S;x < PGROUNDDOWN(UVM_KVM_E); x+= PGSIZE)
	{
		uint page = vm_findpg(x, 0, k_pgdir, 0);
		uint flags = findpgflags(x, k_pgdir);
		if(!page) continue;
		vm_mappage(page, x, dir, 0, 0);
		if(!(flags & PGTBL_RW))
			pgreadonly(x, dir, 0);
	}

	vm_pop_pgdir(save);
}

void vm_map_uvm(pgdir* dst_dir, pgdir* src_dir)
{
	pgdir* save = vm_push_pgdir();

	uint x;
	for(x = 0;x < (UVM_KVM_S >> 22);x++)
	{   
		uint src_page = PGROUNDDOWN(src_dir[x]); 
		if(!src_page) continue;
		if(!dst_dir[x])dst_dir[x] = palloc() | UTBLFLAGS;
		memmove((void*)PGROUNDDOWN(dst_dir[x]), 
				(void*)PGROUNDDOWN(src_dir[x]), 
				PGSIZE);
	}

	vm_pop_pgdir(save);
}

void vm_copy_uvm(pgdir* dst_dir, pgdir* src_dir)
{
	pgdir* save = vm_push_pgdir();
	uint x;
	for(x = 0;x < UVM_KVM_S;x += PGSIZE)
	{
		uint src_page = vm_findpg(x, 0, src_dir, 1);
		if(!src_page) continue;
		int src_flags = findpgflags(x, src_dir);
		if(src_flags == -1) panic("kernel: page not present?\n");

		uint dst_page = vm_findpg(x, 1, dst_dir, 1);
		memmove((uchar*)dst_page,
				(uchar*)src_page,
				PGSIZE);
		pgflags(x, dst_dir, 1, src_flags);
	}

	/* Create new kstack */
	for(x = PGROUNDDOWN(UVM_KSTACK_S);
			x < PGROUNDUP(UVM_KSTACK_E);
			x += PGSIZE)
	{
		/* Remove old mapping */
		vm_unmappage(x, dst_dir);
		uint src_page = vm_findpg(x, 0, src_dir, 0);
		if(!src_page) continue;

		/* Create and map new page */
		uint dst_page = vm_findpg(x, 1, dst_dir, 0);
		/* Copy the page contents */
		memmove((uchar*)dst_page,
				(uchar*)src_page,
				PGSIZE);
	}
	vm_pop_pgdir(save);
}

void vm_set_user_kstack(pgdir* dir, pgdir* kstack)
{
	pgdir* save = vm_push_pgdir();
	int x;
	for(x = UVM_KSTACK_S;x < UVM_KSTACK_E;x += PGSIZE)
	{
		int kstack_pg = vm_findpg(x, 0, kstack, 0);
		/* Make sure the mapping is clear */
		vm_unmappage(x, dir);
		/* Map the page */
		vm_mappage(kstack_pg, x, dir, 0x0, 0x0);
	}
	vm_pop_pgdir(save);
}

void vm_set_swap_stack(pgdir* dir, pgdir* swap)
{
	pgdir* save = vm_push_pgdir();
	/* Make sure the swap stack is clear */
	vm_clear_swap_stack(dir);

	/* Map the new pages in */
	uint start = PGROUNDDOWN(SVM_KSTACK_S);
	uint end = PGROUNDUP(SVM_KSTACK_E);
	uint pg;
	for(pg = start;pg < end;pg += PGSIZE)
	{
		uint src = vm_findpg(pg + SVM_DISTANCE, 0, swap, 0);
		vm_mappage(src, pg, dir, 0, 0);
	}

	vm_pop_pgdir(save);	
}

void vm_clear_swap_stack(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	uint start = PGROUNDDOWN(SVM_KSTACK_S);
	uint end = PGROUNDUP(SVM_KSTACK_E);
	uint pg;
	for(pg = start;pg < end;pg += PGSIZE)
		vm_unmappage(pg, dir);

	vm_pop_pgdir(save);
}

void vm_free_uvm(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	int x;
	for(x = 0;x < (UVM_KVM_S >> 22);x++)
	{
		/* See if the table is allocated */
		if(!dir[x]) continue;

		uint* table = (uint*)PGROUNDDOWN(dir[x]);

		/* Table is allocated, free entries */
		int entry;
		for(entry = 0;entry < PGSIZE / sizeof(int);entry++)
		{
			if(!table[entry]) continue;
			pfree(table[entry]);
		}

		/* Free the table itself */
		pfree((uint)table);
		dir[x] = 0x0; /* Unset directory entry */
	}

	vm_pop_pgdir(save);
}

void freepgdir(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();
	/* Free user pages */
	vm_free_uvm(dir);

	/* Unmap kernel stack */
	uint pg;
	for(pg = PGROUNDDOWN(UVM_KSTACK_S);
			pg < PGROUNDUP(UVM_KSTACK_E);
			pg += PGSIZE)
	{
		uintptr_t freed = vm_unmappage(pg, dir);
		if(freed) pfree(freed);
	}

	/* Free directory pages */
	uint x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
		if(dir[x]) pfree(dir[x]);

	/* free directory */
	pfree((uint)dir);

	vm_pop_pgdir(save);
}

void freepgdir_struct(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	/* Free directory pages */
	uint x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
		if(dir[x]) pfree(dir[x]);

	/* free directory */
	pfree((uint)dir);

	vm_pop_pgdir(save);
}
