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

/* Page directory flags */
#define PGDIR_PRSNT (1 << 0x0)
#define PGDIR_WRITE (1 << 0x1)
#define PGDIR_USERP (1 << 0x2)
#define PGDIR_WRTHR (1 << 0x3)
#define PGDIR_CACHD (1 << 0x4)
#define PGDIR_ACESS (1 << 0x5)
#define PGDIR_LGPGS (1 << 0x7)

/* Page table flags */
#define PGTBL_PRSNT (1 << 0x0)
#define PGTBL_WRITE (1 << 0x1)
#define PGTBL_USERP (1 << 0x2)
#define PGTBL_WRTHR (1 << 0x3)
#define PGTBL_CACHD (1 << 0x4)
#define PGTBL_ACESS (1 << 0x5)
#define PGTBL_DIRTY (1 << 0x6)
#define PGTBL_GLOBL (1 << 0x8)

/** Default kernel directory and table flags for i386 */
#define DEFAULT_DIRFLAGS (VM_DIR_PRES)
#define DEFAULT_TBLFLAGS (VM_TBL_PRES)

uchar __kvm_stack_check__(void);
void __kvm_swap__(pgdir* kvm);
uchar __check_paging__(void);

extern pgdir* k_pgdir;
void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user, uint flags);

void vm_clear_uvm_kstack(pgdir* dir);
void vm_set_uvm_kstack(pgdir* dir, pgdir* swap);

pgflags_t vm_dir_flags(pgflags_t flags)
{
	pgflags_t result = 0;
        if(flags & VM_DIR_ACSS)
		result |= PGDIR_ACESS;
        if(flags & VM_DIR_CACH)
		result |= PGDIR_CACHD;
        if(flags & VM_DIR_WRTR)
		result |= PGDIR_WRTHR;
        if(flags & VM_DIR_USRP)
		result |= PGDIR_USERP;
        if(flags & VM_DIR_WRIT)
		result |= PGDIR_WRITE;
        if(flags & VM_DIR_PRES)
		result |= PGDIR_PRSNT;
        if(flags & VM_DIR_LRGP)
		result |= PGDIR_LGPGS;

	return result;
}

pgflags_t vm_tbl_flags(pgflags_t flags)
{
        pgflags_t result = 0;
        if(flags & VM_TBL_GLBL)
		result |= PGTBL_GLOBL;
        if(flags & VM_TBL_DRTY)
		result |= PGTBL_DIRTY;
        if(flags & VM_TBL_ACSS)
		result |= PGTBL_ACESS;
        if(flags & VM_TBL_CACH)
		result |= PGTBL_CACHD;
        if(flags & VM_TBL_WRTH)
		result |= PGTBL_WRTHR;
        if(flags & VM_TBL_USRP)
		result |= PGTBL_USERP;
        if(flags & VM_TBL_WRIT)
		result |= PGTBL_WRITE;
        if(flags & VM_TBL_PRES)
		result |= PGTBL_DIRTY;

	return result;
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

int vm_mappage(uintptr_t phy, uintptr_t virt, pgdir* dir, 
		pgflags_t dir_flags, pgflags_t tbl_flags)
{
        pgdir* save = vm_push_pgdir();

        dir_flags = vm_dir_flags(DEFAULT_DIRFLAGS | dir_flags);
        tbl_flags = vm_tbl_flags(DEFAULT_TBLFLAGS | tbl_flags);

        phy = PGROUNDDOWN(phy);
        virt = PGROUNDDOWN(virt);
        int dir_index = PGDIRINDEX(virt);
        int tbl_index = PGTBLINDEX(virt);
        /* Do we need to allocate a new page table? */
        if(!dir[dir_index]) dir[dir_index] = palloc() | dir_flags;

        pgtbl* tbl = (pgtbl*)(PGROUNDDOWN(dir[dir_index]));
        if(!tbl[tbl_index])
        {
                tbl[tbl_index] = phy | tbl_flags;
        } else {
                panic("kvm remap: 0x%x\n", virt);
		return -1;
        }

        vm_pop_pgdir(save);

        return 0;
}

int vm_mappages(uintptr_t va, size_t sz, pgdir* dir, 
	pgflags_t dir_flags, pgflags_t tbl_flags)
{
	pgdir* save = vm_push_pgdir();
        /* round va + sz up to a page */
        uintptr_t end = PGROUNDDOWN((va + sz + PGSIZE - 1));
        uintptr_t start = PGROUNDDOWN(va);

        if(end <= start) 
	{
		vm_pop_pgdir(save);
		return -1;
	}

        uintptr_t x;
        for(x = start;x != end;x += PGSIZE)
        {
		uintptr_t page = palloc();
		if(!page) return -1;
		if(vm_mappage(page, x, dir, dir_flags, tbl_flags))
			return -1;
        }

	vm_pop_pgdir(save);

	return 0;
}

int vm_dir_mappages(uintptr_t start, uintptr_t end, pgdir* dir, 
	pgflags_t dir_flags, pgflags_t tbl_flags)
{
	pgdir* save = vm_push_pgdir();

        start = PGROUNDDOWN(start);
        end = PGROUNDUP(end);
        uint x;
        for(x = start;x < end;x += PGSIZE)
                if(vm_mappage(x, x, dir, dir_flags, tbl_flags))
			return -1;

	vm_pop_pgdir(save);

	return 0;
}

uintptr_t vm_unmappage(uintptr_t virt, pgdir* dir)
{
        pgdir* save = vm_push_pgdir();

        virt = PGROUNDDOWN(virt);
        int dir_index = PGDIRINDEX(virt);
        int tbl_index = PGTBLINDEX(virt);

        if(!dir[dir_index])
        {
                vm_pop_pgdir(save);
                return 0;
        }

        pgtbl* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
        if(tbl[tbl_index])
        {
                uintptr_t page = PGROUNDDOWN(tbl[tbl_index]);
                tbl[tbl_index] = 0;
                vm_pop_pgdir(save);
                return page;
        }

        vm_pop_pgdir(save);

        return 0;
}

uintptr_t vm_findpg(uintptr_t virt, int create, pgdir* dir,
	pgflags_t dir_flags, pgflags_t tbl_flags)
{
	pgdir* save = vm_push_pgdir();

	dir_flags = vm_dir_flags(DEFAULT_DIRFLAGS | dir_flags);
        tbl_flags = vm_tbl_flags(DEFAULT_TBLFLAGS | tbl_flags);

        virt = PGROUNDDOWN(virt);
       	int dir_index = PGDIRINDEX(virt);
        int tbl_index = PGTBLINDEX(virt);

        if(!dir[dir_index])
        {
                if(create) 
		{
			dir[dir_index] = palloc() | dir_flags;
                } else {
			vm_pop_pgdir(save);
			return 0;
		}
        }

        pgtbl* tbl = (uint*)(PGROUNDDOWN(dir[dir_index]));
        uintptr_t page;
        if(!(page = tbl[tbl_index]))
        {
                if(create) 
		{
			page = tbl[tbl_index] = palloc() | tbl_flags;
                } else {
			vm_pop_pgdir(save);
			return 0;
		}
        }

	vm_pop_pgdir(save);
        return PGROUNDDOWN(page);
}

pgflags_t vm_findpgflags(uintptr_t virt, pgdir* dir)
{
        pgdir* save = vm_push_pgdir();

        virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return 0;
	}

	pgtbl* tbl = (pgtbl*)(PGROUNDDOWN(dir[dir_index]));
	uintptr_t page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return 0;
	}

	vm_pop_pgdir(save);

	return page & (PGSIZE - 1);
}

pgflags_t vm_findtblflags(uintptr_t virt, pgdir* dir)
{
        pgdir* save = vm_push_pgdir();

        virt = PGROUNDDOWN(virt);
        int dir_index = PGDIRINDEX(virt);

        if(!dir[dir_index])
        {
                vm_pop_pgdir(save);
                return 0;
        }
	
	return dir[dir_index] & (PGSIZE - 1);
}

int vm_pgflags(uintptr_t virt, pgdir* dir, pgflags_t tbl_flags)
{
	pgdir* save = vm_push_pgdir();
	tbl_flags = vm_tbl_flags(DEFAULT_TBLFLAGS | tbl_flags);

	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return -1;
	}

	pgtbl* tbl = (pgtbl*)(PGROUNDDOWN(dir[dir_index]));
	intptr_t page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return -1;
	} else {
		page = PGROUNDDOWN(page);
		page |= tbl_flags;
		tbl[tbl_index] = page;
	}

	vm_pop_pgdir(save);

	return 0;
}


int vm_pgreadonly(uintptr_t virt, pgdir* dir)
{
        pgdir* save = vm_push_pgdir();

        virt = PGROUNDDOWN(virt);
        int dir_index = PGDIRINDEX(virt);
        int tbl_index = PGTBLINDEX(virt);

        if(!dir[dir_index])
        {
                vm_pop_pgdir(save);
                return -1;
        }

        pgtbl* tbl = (pgtbl*)(PGROUNDDOWN(dir[dir_index]));
        intptr_t page;
        if(!(page = tbl[tbl_index]))
        {
                vm_pop_pgdir(save);
                return -1;
        } else {
                page &= ~(PGTBL_WRITE);
        }

        vm_pop_pgdir(save);

        return 0;
}

int vm_pgsreadonly(uintptr_t start, uintptr_t end, pgdir* dir)
{
	uintptr_t virt_start = PGROUNDDOWN(start);
	uintptr_t virt_end = PGROUNDUP(end);

	uintptr_t addr = virt_start;
	for(;addr < virt_end;addr += PGSIZE)
		if(vm_pgreadonly(addr, dir))
			return -1;

	return 0;
}

size_t vm_memmove(void* dst, const void* src, size_t sz, 
		pgdir* dst_pgdir, pgdir* src_pgdir,
		pgflags_t dir_flags, pgflags_t tbl_flags)
{
	if(sz == 0) return 0;
	pgdir* save = vm_push_pgdir();
	size_t bytes = 0; /* bytes copied */

	while(bytes != sz)
	{
		/* How many bytes are there left to copy? */
		uint left = sz - bytes;
		int to_copy = left;

		/* The source page */
		uintptr_t src_page = vm_findpg((uintptr_t)src + bytes, 0,
			 src_pgdir, 0, 0);
		off_t src_offset = ((uintptr_t)src + bytes) & (PGSIZE - 1);
		size_t src_cpy = PGSIZE - src_offset;
		if(!src_page) break;

		/* Check destination page */
		uintptr_t dst_page = vm_findpg((uintptr_t)dst + bytes, 1,
			 dst_pgdir, dir_flags, tbl_flags);
		off_t dst_offset = ((uintptr_t)dst + bytes) & (PGSIZE - 1);
		size_t dst_cpy = PGSIZE - dst_offset;
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

int vm_copy_kvm(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	uintptr_t x;
	for(x = UVM_KVM_S;x < PGROUNDDOWN(UVM_KVM_E); x+= PGSIZE)
	{
		uintptr_t page = vm_findpg(x, 0, k_pgdir, 0, 0);
		uintptr_t pg_flags = vm_findpgflags(x, k_pgdir);
		uintptr_t tbl_flags = vm_findtblflags(x, k_pgdir);
		if(!page) continue;

		if(vm_mappage(page, x, dir, tbl_flags, pg_flags))
			return -1;
	}

	vm_pop_pgdir(save);

	return 0;
}

void vm_map_uvm(pgdir* dst_dir, pgdir* src_dir)
{
	pgdir* save = vm_push_pgdir();

	uintptr_t x;
	for(x = 0;x < (UVM_KVM_S >> 22);x++)
	{   
		uintptr_t src_page = PGROUNDDOWN(src_dir[x]); 
		pgflags_t tbl_flags = vm_findtblflags(src_page, src_dir);
		if(!src_page) continue;
		if(!dst_dir[x]) dst_dir[x] = palloc() | tbl_flags;
		memmove((void*)PGROUNDDOWN(dst_dir[x]), 
				(void*)PGROUNDDOWN(src_dir[x]), 
				PGSIZE);
	}

	vm_pop_pgdir(save);
}

void vm_copy_uvm(pgdir* dst_dir, pgdir* src_dir)
{
	pgdir* save = vm_push_pgdir();
	uintptr_t x;
	for(x = 0;x < UVM_KVM_S;x += PGSIZE)
	{
		uintptr_t src_page = vm_findpg(x, 0, src_dir, 0, 0);
		if(!src_page) continue;
		pgflags_t src_pgflags = vm_findpgflags(x, src_dir);
		pgflags_t src_tblflags = vm_findtblflags(x, src_dir);
		
		uintptr_t dst_page = vm_findpg(x, 1, dst_dir, 
			src_tblflags, src_pgflags);
		memmove((void*)dst_page, (void*)src_page, PGSIZE);
	}

	/* Create new kstack */
	for(x = PGROUNDDOWN(UVM_KSTACK_S);
			x < PGROUNDUP(UVM_KSTACK_E);
			x += PGSIZE)
	{
		/* Remove old mapping */
		vm_unmappage(x, dst_dir);
		uintptr_t src_page = vm_findpg(x, 0, src_dir, 0, 0);
		if(!src_page) continue;

		/* Create and map new page */
		uintptr_t dst_page = vm_findpg(x, 1, dst_dir, 0, 0);
		/* Copy the page contents */
		memmove((void*)dst_page, (void*)src_page, PGSIZE);
	}

	vm_pop_pgdir(save);
}

void vm_set_user_kstack(pgdir* dir, pgdir* kstack)
{
	pgdir* save = vm_push_pgdir();
	int x;
	for(x = UVM_KSTACK_S;x < UVM_KSTACK_E;x += PGSIZE)
	{
		uintptr_t kstack_pg = vm_findpg(x, 0, kstack, 0, 0);
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
	uintptr_t start = PGROUNDDOWN(SVM_KSTACK_S);
	uintptr_t end = PGROUNDUP(SVM_KSTACK_E);
	uintptr_t pg;
	for(pg = start;pg < end;pg += PGSIZE)
	{
		uintptr_t src = vm_findpg(pg + SVM_DISTANCE, 0, swap, 0, 0);
		vm_mappage(src, pg, dir, 0, 0);
	}

	vm_pop_pgdir(save);	
}

void vm_clear_swap_stack(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	uintptr_t start = PGROUNDDOWN(SVM_KSTACK_S);
	uintptr_t end = PGROUNDUP(SVM_KSTACK_E);
	uintptr_t pg;
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

		uintptr_t* table = (uintptr_t*)PGROUNDDOWN(dir[x]);

		/* Table is allocated, free entries */
		int entry;
		for(entry = 0;entry < PGSIZE / sizeof(pgtbl);entry++)
		{
			if(!table[entry]) continue;
			pfree(table[entry]);
		}

		/* Free the table itself */
		pfree((uintptr_t)table);
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
	uintptr_t pg;
	for(pg = PGROUNDDOWN(UVM_KSTACK_S);
			pg < PGROUNDUP(UVM_KSTACK_E);
			pg += PGSIZE)
	{
		uintptr_t freed = vm_unmappage(pg, dir);
		if(freed) pfree(freed);
	}

	/* Free directory pages */
	uintptr_t x;
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
	uintptr_t x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
		if(dir[x]) pfree(dir[x]);

	/* free directory */
	pfree((uintptr_t)dir);

	vm_pop_pgdir(save);
}
