#include "types.h"
#include "x86.h"
#include "file.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "cpu.h"

#define KDIRFLAGS (PGDIR_PRESENT | PGDIR_RW)
#define KTBLFLAGS (PGTBL_PRESENT | PGTBL_RW)
#define UDIRFLAGS (PGDIR_PRESENT | PGDIR_RW | PGDIR_USER)
#define UTBLFLAGS (PGTBL_PRESENT | PGTBL_RW | PGTBL_USER)

uchar __kvm_stack_check__(void);
void __kvm_swap__(pgdir* kvm);
uchar __check_paging__(void);
void __enable_paging__(pgdir* dir);
pgdir* __get_cr3__(void);

extern pgdir* k_pgdir;
void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user, uint flags);
void vm_safe_mapping(uint start, uint end, pgdir* dir);

void vm_clear_uvm_kstack(pgdir* dir);
void vm_set_uvm_kstack(pgdir* dir, pgdir* swap);
pgdir* vm_push_pgdir(void)
{
	/* Is paging even enabled? */
	if(!__check_paging__()) return NULL;
	pgdir* dir = __get_cr3__();
	if(dir == k_pgdir) return NULL;
	push_cli();
	/* Are we on the kernel stack? */
	if(!__kvm_stack_check__())
		__kvm_swap__(k_pgdir);
	else __enable_paging__(k_pgdir);
	return dir;	
}

void vm_pop_pgdir(pgdir* dir)
{
	if(dir == NULL) return;
	if(!__check_paging__()) return;
        if(dir == k_pgdir) return;
	//vm_clear_uvm_kstack(k_pgdir);
	__enable_paging__(dir);
	pop_cli();
}

void mappages(uint va, uint sz, pgdir* dir, uchar user)
{
	pgdir* save = vm_push_pgdir();
        /* round va + sz up to a page */
        uint end = PGROUNDDOWN((va + sz + PGSIZE - 1));
        uint start = PGROUNDDOWN(va);
        if(debug)
                cprintf("Creating virtual mapping from 0x%x to 0x%x\n",
                                start, end);
        if(end <= start) 
	{
		vm_pop_pgdir(save);
		return;
	}
        uint x;
        for(x = start;x != end;x += PGSIZE)
        {
                uint pg = findpg(x, 1, dir, user);
                if(debug) cprintf("Virtual page 0x%x mapped to 0x%x\n", x, pg);
        }
	vm_pop_pgdir(save);
}

void dir_mappages(uint start, uint end, pgdir* dir, uchar user)
{
	pgdir* save = vm_push_pgdir();
        start = PGROUNDDOWN(start);
        end = PGROUNDUP(end);
        uint x;
        for(x = start;x < end;x += PGSIZE)
                mappage(x, x, dir, user, 0);
	vm_pop_pgdir(save);
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
                if(!findpg(page, 0, k_pgdir, 0))
                        mappage(page, page, k_pgdir, 0, 0);
        }
	vm_pop_pgdir(save);
}

void mapping(uint phy, uint virt, uint sz, pgdir* dir, uchar user, uint flags)
{
	pgdir* save = vm_push_pgdir();
        phy = PGROUNDDOWN(phy);
        virt = PGROUNDDOWN(virt);
        uint end_phy = PGROUNDDOWN(phy + sz + PGSIZE - 1);
        uint end_virt = PGROUNDDOWN(virt + sz + PGSIZE - 1);
        uint pages = end_phy - phy;
        if(end_virt - virt > end_phy - phy)
                pages = end_virt - virt;
        pages /= PGSIZE;

        uint x;
        for(x = 0;x < pages;x++)
                mappage(phy + x, virt + x, dir, user, flags);
	 vm_pop_pgdir(save);
}

void mappage(uint phy, uint virt, pgdir* dir, uchar user, uint flags)
{
	pgdir* save = vm_push_pgdir();
        uint dir_flags = KDIRFLAGS;
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
}

uint findpg(uint virt, int create, pgdir* dir, uchar user)
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

uint unmappage(uint virt, pgdir* dir)
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

void vm_copy_kvm(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();

	uint x;
	for(x = UVM_KVM_S;x < PGROUNDDOWN(UVM_KVM_E); x+= PGSIZE)
	{
		uint page = findpg(x, 0, k_pgdir, 0);
		if(!page) continue;
		mappage(page, x, dir, 0, 0);
	}

	vm_pop_pgdir(save);
}

void vm_copy_uvm(pgdir* dst_dir, pgdir* src_dir)
{
	pgdir* save = vm_push_pgdir();
	uint x;
	for(x = 0;x < UVM_KVM_S;x += PGSIZE)
	{
		uint src_page = findpg(x, 0, src_dir, 1);
		if(!src_page) continue;

		uint dst_page = findpg(x, 1, dst_dir, 1);
		memmove((uchar*)dst_page,
				(uchar*)src_page,
				PGSIZE);
	}

	/* Create new kstack */
	for(x = PGROUNDDOWN(UVM_KSTACK_S);
		x < PGROUNDUP(UVM_KSTACK_E);
		x += PGSIZE)
	{
		/* Remove old mapping */
		unmappage(x, dst_dir);
		uint src_page = findpg(x, 0, src_dir, 0);
                if(!src_page) continue;

		/* Create and map new page */
                uint dst_page = findpg(x, 1, dst_dir, 0);
		/* Copy the page contents */
                memmove((uchar*)dst_page,
                                (uchar*)src_page,
                                PGSIZE);
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
                uint src = findpg(pg + SVM_DISTANCE, 0, swap, 0);
		mappage(src, pg, dir, 0, 0);
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
		unmappage(pg, dir);

	vm_pop_pgdir(save);
}

void vm_free_uvm(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();
	uint x;
	for(x = 0;x < PGROUNDUP(UVM_KVM_S); x+= PGSIZE)
	{
		uint pg = unmappage(x, dir);
		if(pg) pfree(pg);
	}

	vm_pop_pgdir(save);
}

void freepgdir(pgdir* dir)
{
	pgdir* save = vm_push_pgdir();
	/* Free user pages */
	vm_free_uvm(dir);
	/* Free kernel pages */
	uint x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
	{
		if(dir[x])
		{
			pfree(PGROUNDDOWN(dir[x]));
			dir[x] = 0;
		}
	}

	/* free directory */
	pfree((uint)dir);

	vm_pop_pgdir(save);
}
