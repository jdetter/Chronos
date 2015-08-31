#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "stdarg.h"
#include "stdlib.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "cpu.h"
#include "x86.h"
#include "panic.h"

struct ramfs_context {
	uchar allocated; /* Whether or not this context is in use. */
	uint start_addr; /* Address of first sector (safety)*/
	uint end_addr; /* Last address (safety) */
	uint sectors; /* How many emulated sectors there are */
	pgdir* dir; /* The page directory for this fs */
	pgdir* saved_dir; /* The previous page directory */
	slock_t ram_lock; /* Lock for this context */
	uint bsize; /* sector size */
	struct FSHardwareDriver* driver; /* see table below */
};

#define RAMFS_TABLE_SIZE 0x04
slock_t ramfs_table_lock;
struct ramfs_context ramfs_table[RAMFS_TABLE_SIZE];
struct FSHardwareDriver ramfs_driver_table[RAMFS_TABLE_SIZE];

void ramfs_init(void)
{
	slock_init(&ramfs_table_lock);
	memset(ramfs_table, 0, sizeof(struct ramfs_context) 
		* RAMFS_TABLE_SIZE);
	memset(ramfs_driver_table, 0, 
		sizeof(struct FSHardwareDriver) 
		* RAMFS_TABLE_SIZE);
}

static struct FSHardwareDriver* ramfs_alloc(void)
{
	slock_acquire(&ramfs_table_lock);
	struct FSHardwareDriver* driver = NULL;
	int x;
	for(x = 0;x < RAMFS_TABLE_SIZE;x++)
	{
		if(!ramfs_driver_table[x].valid)
		{
			ramfs_driver_table[x].valid = 1;
			slock_init(&ramfs_table[x].ram_lock);
			driver = ramfs_driver_table + x;
			driver->context = ramfs_table + x;
			driver->valid = 1;
			break;
		}
	}

	if(driver)
	{
		struct ramfs_context* c = driver->context;
		c->dir = (pgdir*)palloc();
		vm_copy_kvm(c->dir);
		c->allocated = 1;
	}

	slock_release(&ramfs_table_lock);
	return driver;
}

static void ramfs_free(struct FSHardwareDriver* driver)
{
	slock_acquire(&ramfs_table_lock);
	struct ramfs_context* c = driver->context;
	freepgdir(c->dir);
	c->allocated = 0;
	driver->valid = 0;
	slock_release(&ramfs_table_lock);
}

void __enable_paging__(uint* pgdir);
uint __get_cr3__(void);
static void ramfs_enter(struct ramfs_context* c)
{
	pgdir* old = (pgdir*)__get_cr3__();
	vm_set_user_kstack(c->dir, old);
	c->saved_dir = old;
	/* Switch to our context */
	push_cli();
	__enable_paging__(c->dir);
	pop_cli();
}

static void ramfs_exit(struct ramfs_context* c)
{
	/* Just restore old address space */
	push_cli();
	__enable_paging__(c->saved_dir);
	pop_cli();	
}

int ramfs_read(void* dst, uint sect, struct FSHardwareDriver* driver);
int ramfs_write(void* dst, uint sect, struct FSHardwareDriver* driver);
struct FSHardwareDriver* ramfs_driver_alloc(uint block_size, uint blocks)
{
	if(block_size == 0 || blocks == 0) return NULL;

	/* Try to get a context */
	struct FSHardwareDriver* driver = ramfs_alloc();
	if(!driver) return NULL;
	struct ramfs_context* context = driver->context;

	uint start = 0x1000;
	uint end = PGROUNDUP(block_size * blocks);

	mappages(start, end - start, context->dir, 0);

	context->start_addr = start;
	context->end_addr = end;
	context->sectors = (end - start) / block_size;
	context->bsize = block_size;

	driver->readsect = ramfs_read;
	driver->writesect = ramfs_write;
	driver->context = context;
	driver->valid = 1;

	return driver;
}

int ramfs_driver_free(struct FSHardwareDriver* driver)
{
	ramfs_free(driver);
	return 0;
}

int ramfs_read(void* dst, uint sect, struct FSHardwareDriver* driver)
{
	struct ramfs_context* context = driver->context;
	/* Acquire lock */
	slock_acquire(&context->ram_lock);
	/* Enter address space */
	ramfs_enter(context);

	uint start = (sect * context->bsize) + context->start_addr;
	uint end = start + context->bsize;

	if(end > context->end_addr || start < context->start_addr)
		return 1; /* Out of bounds*/

	memmove(dst, (uchar*)start, context->bsize);

	/* Restore old address space */
	ramfs_exit(context);
	slock_release(&context->ram_lock);
	return 0;
}

int ramfs_write(void* dst, uint sect, struct FSHardwareDriver* driver)
{
        struct ramfs_context* context = driver->context;
        /* Acquire lock */
        slock_acquire(&context->ram_lock);
        /* Enter address space */
        ramfs_enter(context);

        uint start = (sect * context->bsize) + context->start_addr;
        uint end = start + context->bsize;

        if(end > context->end_addr || start < context->start_addr)
                return 1; /* Out of bounds*/

        memmove((uchar*)start, dst, context->bsize);

        /* Restore old address space */
        ramfs_exit(context);
        slock_release(&context->ram_lock);
        return 0;
}
