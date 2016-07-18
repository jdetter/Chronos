/**
 * Author: John Detter <john@detter.com>
 *
 * Page directory functions.
 */

#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
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
#include "k/vm.h"
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

#ifdef __ALLOW_VM_SHARE__
/* There are 3 available page table flags on x86. They are allocated below: */
#define PGTBL_SHARE (1 << 0x9) /* Marks the page as shared */
#define PGTBL_CONWR (1 << 0xa) /* Marks the page is copy on write (cow) */
#endif

/** Default kernel directory and table flags for i386 */
#define DEFAULT_DIRFLAGS (VM_DIR_PRES)
#define DEFAULT_TBLFLAGS (VM_TBL_PRES)

/** Is paging enabled? */
int __kvm_stack_check__(void);
/** Switch to kernel memory and stack */
void __kvm_swap__(pgdir_t* kvm);

extern pgdir_t* k_pgdir;

/**
 * Turn generic flags for a page directory into flags for an i386
 * page directory. Returns the correcponding flags for an i386 page
 * table.
 */
static vmflags_t vm_dir_flags(vmflags_t flags)
{
	vmflags_t result = 0;
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

/**
 * Turn i386 flags for a page directory into generic vm flags.
 */
static vmflags_t vm_gen_dir_flags(vmflags_t flags)
{
	vmflags_t result = 0;
	if(flags & PGDIR_ACESS)
		result |= VM_DIR_ACSS;
	if(flags & PGDIR_CACHD)
		result |= VM_DIR_CACH;
	if(flags & PGDIR_WRTHR)
		result |= VM_DIR_WRTR;
	if(flags & PGDIR_USERP)
		result |= VM_DIR_USRP;
	if(flags & PGDIR_WRITE)
		result |= VM_DIR_WRIT;
	if(flags & PGDIR_PRSNT)
		result |= VM_DIR_PRES;
	if(flags & PGDIR_LGPGS)
		result |= VM_DIR_LRGP;

	/* Read cannot be disabled on i386 . */
	result |= VM_DIR_READ;

	return result;
}

/**
 * Turn generic flags for a page table into flags for an i386
 * page table. Returns the corresponding flags for an i386 page
 * table.
 */
static vmflags_t vm_tbl_flags(vmflags_t flags)
{
	vmflags_t result = 0;
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
		result |= PGTBL_PRSNT;

#ifdef __ALLOW_VM_SHARE__
	if(flags & VM_TBL_SHAR)
		result |= PGTBL_SHARE;
	if(flags & VM_TBL_COWR)
	{
		result |= PGTBL_CONWR;
		result &= ~(PGTBL_WRITE);
	}
#endif

	return result;
}

static vmflags_t vm_gen_tbl_flags(vmflags_t flags)
{
	vmflags_t result = 0;
	if(flags & PGTBL_GLOBL)
		result |= VM_TBL_GLBL;
	if(flags & PGTBL_DIRTY)
		result |= VM_TBL_DRTY;
	if(flags & PGTBL_ACESS)
		result |= VM_TBL_ACSS;
	if(flags & PGTBL_CACHD)
		result |= VM_TBL_CACH;
	if(flags & PGTBL_WRTHR)
		result |= VM_TBL_WRTH;
	if(flags & PGTBL_USERP)
		result |= VM_TBL_USRP;
	if(flags & PGTBL_WRITE)
		result |= VM_TBL_WRIT;
	if(flags & PGTBL_PRSNT)
		result |= VM_TBL_PRES;

#ifdef __ALLOW_VM_SHARE__
	if(flags & PGTBL_SHARE)
		result |= VM_TBL_SHAR;
	if(flags & PGTBL_CONWR)
	{
		result |= VM_TBL_COWR;
		result &= ~(VM_TBL_WRIT);
	}
#endif

	/* All pages are read enabled in i386 */
	flags |= VM_TBL_READ;
	/* All pages are also executable in i386 */
	flags |= VM_TBL_EXEC;

	return result;
}

pgdir_t* vm_push_pgdir(void)
{
	/* If we are modifying the address space, turn of interrupts. */
	push_cli();

	/* Is paging even enabled? */
	if(!vm_check_paging()) return NULL;
	pgdir_t* dir = vm_curr_pgdir();
	if(dir == k_pgdir) return NULL;

	/* Are we on the kernel stack? */
	if(!__kvm_stack_check__())
		__kvm_swap__(k_pgdir);
	else vm_enable_paging(k_pgdir);
	return dir;	
}

void vm_pop_pgdir(pgdir_t* dir)
{
	pop_cli();
	if(dir == NULL) return;
	if(!vm_check_paging()) return;
	if(dir == k_pgdir) return;
	vm_enable_paging(dir);
}

static int vm_mappage_native(pypage_t phy, vmpage_t virt, pgdir_t* dir,
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	pgdir_t* save = vm_push_pgdir();

	phy = PGROUNDDOWN(phy);
	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);
	/* Do we need to allocate a new page table? */
	if(!dir[dir_index]) dir[dir_index] = palloc() | dir_flags;

	pgtbl_t* tbl = (pgtbl_t*)(PGROUNDDOWN(dir[dir_index]));
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

int vm_mappage(pypage_t phy, vmpage_t virt, pgdir_t* dir, 
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	dir_flags = vm_dir_flags(DEFAULT_DIRFLAGS | dir_flags);
	tbl_flags = vm_tbl_flags(DEFAULT_TBLFLAGS | tbl_flags);

	return vm_mappage_native(phy, virt, dir, dir_flags, tbl_flags);
}

int vm_mappages(vmpage_t va, size_t sz, pgdir_t* dir, 
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	pgdir_t* save = vm_push_pgdir();
	/* round va + sz up to a page */
	vmpage_t end = PGROUNDUP(va + sz);
	vmpage_t start = PGROUNDDOWN(va);

	if(end <= start) 
	{
		vm_pop_pgdir(save);
		return -1;
	}

	vmpage_t x;
	for(x = start;x != end;x += PGSIZE)
	{
		vmpage_t page = palloc();
		if(!page) return -1;
		if(vm_mappage(page, x, dir, dir_flags, tbl_flags))
			return -1;
	}

	vm_pop_pgdir(save);

	return 0;
}

int vm_dir_mappages(vmpage_t start, vmpage_t end, pgdir_t* dir, 
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	pgdir_t* save = vm_push_pgdir();

	start = PGROUNDDOWN(start);
	end = PGROUNDUP(end);
	vmpage_t x;
	for(x = start;x < end;x += PGSIZE)
		if(vm_mappage(x, x, dir, dir_flags, tbl_flags))
			return -1;
	vm_pop_pgdir(save);

	return 0;
}

vmpage_t vm_unmappage(vmpage_t virt, pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return 0;
	}

	pgtbl_t* tbl = (pgtbl_t*)(PGROUNDDOWN(dir[dir_index]));
	if(tbl[tbl_index])
	{
		vmpage_t page = PGROUNDDOWN(tbl[tbl_index]);
		tbl[tbl_index] = 0;
		vm_pop_pgdir(save);

		return page;
	}

	vm_pop_pgdir(save);

	return 0;
}

int vm_cpy_page(pypage_t dst, pypage_t src)
{
	pgdir_t* save = vm_push_pgdir();
	char* src_c = (char*)src;
	char* dst_c = (char*)dst;
	memmove(dst_c, src_c, PGSIZE);
	vm_pop_pgdir(save);
	return 0;
}

static vmpage_t vm_findpg_native(vmpage_t virt, int create, pgdir_t* dir,
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	pgdir_t* save = vm_push_pgdir();

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

	pgtbl_t* tbl = (pgtbl_t*)(PGROUNDDOWN(dir[dir_index]));
	vmpage_t page;
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

pypage_t vm_findpg(vmpage_t virt, int create, pgdir_t* dir,
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	dir_flags = vm_dir_flags(DEFAULT_DIRFLAGS | dir_flags);
	tbl_flags = vm_tbl_flags(DEFAULT_TBLFLAGS | tbl_flags);
	return vm_findpg_native(virt, create, dir, dir_flags, tbl_flags);
}

static vmflags_t vm_findpgflags_native(vmpage_t virt, pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return 0;
	}

	pgtbl_t* tbl = (pgtbl_t*)(PGROUNDDOWN(dir[dir_index]));
	vmpage_t page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return 0;
	}

	vm_pop_pgdir(save);

	return page & (PGSIZE - 1);
}

vmflags_t vm_findpgflags(vmpage_t virt, pgdir_t* dir)
{
	return vm_gen_tbl_flags(vm_findpgflags_native(virt, dir));
}

static vmflags_t vm_findtblflags_native(vmpage_t virt, pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return 0;
	}

	vmflags_t result = dir[dir_index] & (PGSIZE - 1); 

	vm_pop_pgdir(save);

	return result;
}

vmflags_t vm_findtblflags(vmpage_t virt, pgdir_t* dir)
{
	return vm_gen_dir_flags(vm_findtblflags_native(virt, dir));
}

int vm_setpgflags(vmpage_t virt, pgdir_t* dir, vmflags_t pg_flags)
{
	pgdir_t* save = vm_push_pgdir();
	pg_flags = vm_tbl_flags(DEFAULT_TBLFLAGS | pg_flags);

	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return -1;
	}

	pgtbl_t* tbl = (pgtbl_t*)(PGROUNDDOWN(dir[dir_index]));
	vmpage_t page;
	if(!(page = tbl[tbl_index]))
	{
		vm_pop_pgdir(save);
		return -1;
	} else {
		page = PGROUNDDOWN(page);
		page |= pg_flags;
		tbl[tbl_index] = page;
	}

	vm_pop_pgdir(save);

	return 0;
}


int vm_pgreadonly(vmpage_t virt, pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	virt = PGROUNDDOWN(virt);
	int dir_index = PGDIRINDEX(virt);
	int tbl_index = PGTBLINDEX(virt);

	if(!dir[dir_index])
	{
		vm_pop_pgdir(save);
		return -1;
	}

	pgtbl_t* tbl = (pgtbl_t*)(PGROUNDDOWN(dir[dir_index]));
	vmpage_t page;
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

int vm_pgsreadonly(vmpage_t start, vmpage_t end, pgdir_t* dir)
{
	vmpage_t virt_start = PGROUNDDOWN(start);
	vmpage_t virt_end = PGROUNDUP(end);

	vmpage_t addr = virt_start;
	for(;addr < virt_end;addr += PGSIZE)
		if(vm_pgreadonly(addr, dir))
			return -1;

	return 0;
}

size_t vm_memmove(void* dst, const void* src, size_t sz, 
		pgdir_t* dst_pgdir, pgdir_t* src_pgdir,
		vmflags_t dir_flags, vmflags_t tbl_flags)
{
	/* Check to see if this is just a normal transfer */
	if((vm_curr_pgdir() == dst_pgdir) &&
			dst_pgdir == src_pgdir)
	{
		memmove(dst, src, sz);
		return sz;
	}

	/* Do we need to transfer anything? */
	if(sz == 0) return 0;

	pgdir_t * save = vm_push_pgdir();
	size_t bytes = 0; /* bytes copied */

	while(bytes != sz)
	{
		/* How many bytes are there left to copy? */
		size_t left = sz - bytes;
		size_t to_copy = left;

		/* The source page */
		vmpage_t src_page = vm_findpg((vmpage_t)src + bytes, 0,
				src_pgdir, 0, 0);
		size_t src_offset = ((vmpage_t)src + bytes) & (PGSIZE - 1);
		size_t src_cpy = PGSIZE - src_offset;
		if(!src_page) break;

		/* Check destination page */
		vmpage_t dst_page = vm_findpg((vmpage_t)dst + bytes, 1,
				dst_pgdir, dir_flags, tbl_flags);
		size_t dst_offset = ((vmpage_t)dst + bytes) & (PGSIZE - 1);
		size_t dst_cpy = PGSIZE - dst_offset;
		if(!dst_page) break;

		/* Make sure copy amount is okay */
		if(src_cpy < to_copy) to_copy = src_cpy;
		if(dst_cpy < to_copy) to_copy = dst_cpy;
		memmove((void*)(dst_page + dst_offset),
				(void*)(src_page + src_offset), to_copy);
		/* Adjust bytes */
		bytes += to_copy;
	}

	vm_pop_pgdir(save);
	return bytes;
}

int vm_copy_kvm(pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	vmpage_t x;
	for(x = UVM_KVM_S;x < PGROUNDDOWN(UVM_KVM_E); x+= PGSIZE)
	{
		vmpage_t page = vm_findpg(x, 0, k_pgdir, 0, 0);
		vmflags_t pg_flags = vm_findpgflags_native(x, k_pgdir);
		vmflags_t tbl_flags = vm_findtblflags_native(x, k_pgdir);
		if(!page) continue;

		/* Don't map in the user stack or the swap stack */
		if(x >= SVM_KSTACK_S && x < SVM_KSTACK_E)
			continue;
		if(x >= UVM_KSTACK_S && x < UVM_KSTACK_E)
			continue;


		if(vm_mappage_native(page, x, dir, tbl_flags, pg_flags))
			panic("vm: native map failed.\n");
	}

	vm_pop_pgdir(save);

	return 0;
}

void vm_map_uvm(pgdir_t* dst_dir, pgdir_t* src_dir)
{
	pgdir_t* save = vm_push_pgdir();

	vmpage_t x;
	for(x = 0;x < (UVM_KVM_S >> 22);x++)
	{   
		vmpage_t src_page = PGROUNDDOWN(src_dir[x]); 
		if(!src_page) continue;
		vmflags_t tbl_flags = src_dir[x] & (PGSIZE - 1);

		if(!dst_dir[x]) dst_dir[x] = palloc() | tbl_flags;
		memmove((void*)PGROUNDDOWN(dst_dir[x]), 
				(void*)PGROUNDDOWN(src_dir[x]), 
				PGSIZE);

#ifdef __ALLOW_VM_SHARE__
		pgtbl_t* table = (pgtbl_t*)(PGROUNDDOWN(dst_dir[x]));
		/* Find shared pages */
		int pg_index;
		for(pg_index = 0;pg_index < PGSIZE / sizeof(int);pg_index++)
		{
			unsigned int entry = table[pg_index];
			vmpage_t pg = (x << 22) | (pg_index << 12);
			if(entry & PGTBL_SHARE)
			{
				/* Add a reference count */
				vm_pgshare(pg, dst_dir);
			}
		}
#endif
	}

	vm_pop_pgdir(save);
}

void vm_cpy_user_kstack(pgdir_t* dst_dir, pgdir_t* src_dir)
{
	pgdir_t* save = vm_push_pgdir();

	vmpage_t x;
	/* Create new kstack */
	for(x = PGROUNDDOWN(UVM_KSTACK_S);
			x < PGROUNDUP(UVM_KSTACK_E);
			x += PGSIZE)
	{
		/* Remove old mapping */
		vmpage_t pg = vm_unmappage(x, dst_dir);
		if(pg) pfree(pg);

		vmpage_t src_page = vm_findpg(x, 0, src_dir, 0, 0);
		vmflags_t src_pgflags = vm_findpgflags_native(x, src_dir);
		vmflags_t src_tblflags = vm_findtblflags_native(x, src_dir);
		if(!src_page) continue;

		/* Create and map new page */
		vmpage_t dst_page = vm_findpg_native(x, 1, dst_dir,
				src_tblflags, src_pgflags);
		/* Copy the page contents */
		memmove((void*)dst_page, (void*)src_page, PGSIZE);
	}	

	vm_pop_pgdir(save);	
}

void vm_copy_uvm(pgdir_t* dst_dir, pgdir_t* src_dir)
{
	pgdir_t* save = vm_push_pgdir();
	vmpage_t x;
	for(x = 0;x < UVM_TOP;x += PGSIZE)
	{
		vmpage_t src_page = vm_findpg(x, 0, src_dir, 0, 0);
		if(!src_page) continue;
		vmflags_t src_pgflags = vm_findpgflags_native(x, src_dir);
		vmflags_t src_tblflags = vm_findtblflags_native(x, src_dir);

#ifdef __ALLOW_VM_SHARE__
		/* Is this page shared? */
		if(src_pgflags & PGTBL_SHARE)
		{
			if(vm_mappage_native(src_page, x, dst_dir, 
						src_pgflags, src_tblflags))
				panic("Could not share page!\n");
			/* Add a reference count */
			vm_pgshare(x, dst_dir);
			continue;
		}
#endif

		vmpage_t dst_page = vm_findpg_native(x, 1, dst_dir, 
				src_tblflags, src_pgflags);
		memmove((void*)dst_page, (void*)src_page, PGSIZE);
	}

	/* Map in a new user kstack */
	vm_cpy_user_kstack(dst_dir, src_dir);

	vm_pop_pgdir(save);
}

void vm_set_user_kstack(pgdir_t* dir, pgdir_t* kstack)
{
	pgdir_t* save = vm_push_pgdir();

	int x;
	for(x = UVM_KSTACK_S;x < UVM_KSTACK_E;x += PGSIZE)
	{
		vmpage_t kstack_pg = vm_findpg(x, 0, kstack, 0, 0);
		/* Make sure the mapping is clear */
		vm_unmappage(x, dir);
		/* Map the page */
		vm_mappage(kstack_pg, x, dir, 0x0, 0x0);
	}

	vm_pop_pgdir(save);
}

void vm_set_swap_stack(pgdir_t* dir, pgdir_t* swap)
{
	pgdir_t* save = vm_push_pgdir();
	/* Make sure the swap stack is clear */
	vm_clear_swap_stack(dir);

	/* Map the new pages in */
	vmpage_t start = PGROUNDDOWN(SVM_KSTACK_S);
	vmpage_t end = PGROUNDUP(SVM_KSTACK_E);
	vmpage_t pg;
	for(pg = start;pg < end;pg += PGSIZE)
	{
		vmpage_t src = vm_findpg(pg + SVM_DISTANCE, 0, swap, 0, 0);
		if(!src) 
		{
			panic("vm: invalid swap stack!\n");
		}
		vm_mappage(src, pg, dir, 
				VM_DIR_READ | VM_DIR_WRIT, 
				VM_TBL_READ | VM_TBL_WRIT);
	}

	vm_pop_pgdir(save);	
}

void vm_clear_swap_stack(pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	vmpage_t start = PGROUNDDOWN(SVM_KSTACK_S);
	vmpage_t end = PGROUNDUP(SVM_KSTACK_E);
	vmpage_t pg;
	for(pg = start;pg < end;pg += PGSIZE)
		vm_unmappage(pg, dir);

	vm_pop_pgdir(save);
}

void vm_free_uvm(pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	int x;
	for(x = 0;x < (UVM_KVM_S >> 22);x++)
	{
		/* See if the table is allocated */
		if(!dir[x]) continue;

		pgtbl_t* table = (pgtbl_t*)PGROUNDDOWN(dir[x]);

		/* Table is allocated, free entries */
		int entry;
		for(entry = 0;entry < PGSIZE / sizeof(pgtbl_t);entry++)
		{
			if(!table[entry]) continue;
			pfree(table[entry]);
		}

		/* Free the table itself */
		pfree((vmpage_t)table);
		dir[x] = 0x0; /* Unset directory entry */
	}

	vm_pop_pgdir(save);
}

void freepgdir(pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();
	/* Free user pages */
	vm_free_uvm(dir);

	/* Unmap kernel stack */
	vmpage_t pg;
	for(pg = PGROUNDDOWN(UVM_KSTACK_S);
			pg < PGROUNDUP(UVM_KSTACK_E);
			pg += PGSIZE)
	{
		vmpage_t freed = vm_unmappage(pg, dir);
		if(freed) pfree(freed);
	}

	/* Free directory pages */
	vmpage_t x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
		if(dir[x]) pfree(dir[x]);

	/* free directory */
	pfree((vmpage_t)dir);

	vm_pop_pgdir(save);
}

void vm_freepgdir_struct(pgdir_t* dir)
{
	pgdir_t* save = vm_push_pgdir();

	/* Free directory pages */
	vmpage_t x;
	for(x = 0;x < (PGSIZE / sizeof(uint));x++)
		if(dir[x]) pfree(dir[x]);

	/* free directory */
	pfree((vmpage_t)dir);

	vm_pop_pgdir(save);
}

int vm_print_pgdir(vmpage_t last_page, pgdir_t* dir)
{
	int pgs = 0;
	vmpage_t start = 0;
	vmpage_t end = PGROUNDDOWN(VM_MAX);
	if(last_page) end = last_page;

	int printed = 0;
	for(;start < end;start += PGSIZE)
	{
		vmpage_t page = vm_findpg(start, 0, dir, 0, 0);

		/* If the page is valid, print it out */
		if(page)
		{
			cprintf("0x%x -> 0x%x || ", start, PGROUNDDOWN(page));
			vmflags_t tbl_flags = 
				vm_findtblflags_native(start, dir);
			vmflags_t pg_flags = 
				vm_findpgflags_native(start, dir);

			if(tbl_flags & PGDIR_PRSNT)
				cprintf("PRSNT ");
			if(tbl_flags & PGDIR_WRITE)
				cprintf("WRITE ");
			if(tbl_flags & PGDIR_USERP)
				cprintf("USERP ");
			if(tbl_flags & PGDIR_WRTHR)
				cprintf("WRTHR ");
			if(tbl_flags & PGDIR_CACHD)
				cprintf("CACHD ");
			if(tbl_flags & PGDIR_ACESS)
				cprintf("ACESS ");
			if(tbl_flags & PGDIR_LGPGS)
				cprintf("LGPGS ");

			cprintf("|| ");

			if(pg_flags & PGTBL_PRSNT)
				cprintf("PRSNT ");
			if(pg_flags & PGTBL_WRITE)
				cprintf("WRITE ");
			if(pg_flags & PGTBL_USERP)
				cprintf("USERP ");
			if(pg_flags & PGTBL_WRTHR)
				cprintf("WRTHR ");
			if(pg_flags & PGTBL_CACHD)
				cprintf("CACHD ");
			if(pg_flags & PGTBL_ACESS)
				cprintf("ACESS ");
			if(pg_flags & PGTBL_DIRTY)
				cprintf("DIRTY ");
			if(pg_flags & PGTBL_GLOBL)
				cprintf("GLOBL ");
#ifdef __ALLOW_VM_SHARE__
			if(pg_flags & PGTBL_SHARE)
				cprintf("SHARE ");
			if(pg_flags & PGTBL_CONWR)
				cprintf("CONWR ");
#endif

			cprintf("\n");
			printed = 1;
			pgs++;
		} else {
			if(printed)
			{
				cprintf("...\n");
				printed = 0;
			}
		}
	}

	return pgs;
}
