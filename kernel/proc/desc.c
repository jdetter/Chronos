/**
 * Notes on File Descriptors in Chronos:
 *
 * A file descriptor is a struct that contains some metadata about an open
 * file or device (refered to by a file). Each process in the process table
 * has a pointer to a table of pointers to file descriptors. This B tree
 * like structure allows us to do a couple of cool things. Mainly, it allows
 * processes to share file descriptors and file descriptor tables. Two or more
 * processes can share the same table by just using the same pointer. Two or
 * more processes can also share the same descriptor by having the same
 * pointer entry in their file descriptor tables. The two entries do not
 * have to be in the same position in the respective tables, however they do
 * have to actually point to the same file descriptor to get the described
 * effect. An ascii diagram is provided below:
 *
 *
 * Process 1 Table
 * +------+
 * |/etc/a|----------+
 * |      |          |
 * |      |          |   Main FD Table
 * +------+          |  +------+
 *                 +-+->|/etc/a|
 * Process 2 Table |+-->|/dev/b|
 * +------+        ||   |      |
 * |/etc/a|--------+|   +------+
 * |/dev/b|---------+
 * |      |
 * +------+
 *
 *
 * Author: John Detter <jdetter@chronos.systems>
 *
 */

#include <string.h>
#include <stdlib.h>

#include "stdlock.h"
#include "proc.h"
#include "panic.h"

// #define DEBUG

/* Table for all of the file descriptors */
struct file_descriptor fds[FDS_TABLE_SZ];

/* Table of file descriptor tables */
struct file_descriptor* fd_tables[PTABLE_SIZE][PROC_MAX_FDS];
slock_t fd_tables_locks[PTABLE_SIZE];

/* Lock needed to touch fds */
slock_t fds_lock;

void fd_init(void)
{
	slock_init(&fds_lock);
	memset(fds, 0, sizeof(struct file_descriptor) * FDS_TABLE_SZ);
	memset(fd_tables, 0, sizeof(struct file_descriptor*) * PTABLE_SIZE);

	/* Initilize all locks */
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
		slock_init(fd_tables_locks + x);
}

void fdtab_init(fdtab_t tab)
{
	if(!tab) return;
	memset(tab, 0, sizeof(struct file_descriptor*) * PROC_MAX_FDS);
}

static struct file_descriptor* fd_alloc(void)
{
        struct file_descriptor* result = NULL;
        slock_acquire(&fds_lock);

        int x;
        for(x = 0;x < FDS_TABLE_SZ;x++)
        {
                if(fds[x].type) continue;
                result = fds + x;
		result->type = FD_TYPE_INUSE;
		result->refs = 1;
		slock_init(&result->lock);
                break;
        }

        slock_release(&fds_lock);

        return result;
}

static void fd_free_native(struct file_descriptor * fd)
{
	if(!fd) return;
#ifdef DEBUG
	cprintf("desc: freeing fd: %s\n", fd->path);
	cprintf("desc: ref count old: %d new: %d",
		fd->refs, fd->refs - 1);
#endif
	fd->refs--;
        if(fd->refs <= 0)
                memset(fd, 0, sizeof(struct file_descriptor));
}

void fd_free(struct proc* p, int fd)
{
        if(!p) return;
	
	slock_acquire(p->fdtab_lock);
	struct file_descriptor* file = p->fdtab[fd];
	if(file)
	{
		fd_free_native(p->fdtab[fd]);
		p->fdtab[fd] = NULL;
	}
	slock_release(p->fdtab_lock);
}

int fd_tab_free_native(struct proc* p)
{
	if(!p || !p->fdtab) return -1;
#ifdef DEBUG
        cprintf("desc: freeing table for proc %s\n", p->name);
#endif

	int x;
        for(x = 0;x < PROC_MAX_FDS;x++)
        {
                if(!p->fdtab[x]) continue;
                fd_free_native(p->fdtab[x]);
                p->fdtab[x] = NULL;
        }
	return 0;
}

int fd_tab_free(struct proc* p)
{
	if(!p || !p->fdtab) return -1;
	slock_acquire(p->fdtab_lock);
	int result = fd_tab_free_native(p);
	slock_release(p->fdtab_lock);
	return result;
}

int fd_next_at(struct proc* p, int pos)
{
#ifdef DEBUG
	cprintf("desc: %s is looking for a new fd\n", p->name);
#endif
	int result = -1;
	/* Acquire the fd table lock */
	slock_acquire(p->fdtab_lock);

        int x;
        for(x = pos;x < PROC_MAX_FDS;x++)
        {
		if(p->fdtab[x]) continue;
		/* attempt to get a free fd */
		struct file_descriptor* fd = fd_alloc();
		if(!fd) break;

		/* Set the table entry */
		p->fdtab[x] = fd;
		/* Return this index */
		result = x;
		break;
        }
	
	slock_release(p->fdtab_lock);

#ifdef DEBUG
	cprintf("desc: %s got fd at index %d\n", p->name, result);
#endif
	return result;
}

int fd_tab_map(struct proc* dst, struct proc* src)
{
#ifdef DEBUG
	cprintf("desc: mapping fdtab %s->%s\n", dst->name, src->name);
#endif
	if(!dst || !src) return -1;
	/* Free dst's fdtab */
	if(dst->fdtab)
		fd_tab_free(dst);
	dst->fdtab = src->fdtab;
	dst->fdtab_lock = src->fdtab_lock;
	return 0;
}

int fd_tab_copy(struct proc* dst, struct proc* src)
{
	if(!dst || !src) return -1;
	/* Are both of the tables okay? */
	if(!dst->fdtab || !src->fdtab)
		panic("kernel: invalid fd table! 0x%x 0x%x\n", 
				dst->fdtab, src->fdtab);

	/* Lock both tables */
	/* This hack below is just to prevent possible deadlocks */
	if(dst->fdtab > src->fdtab)
	{
		slock_acquire(src->fdtab_lock);
		slock_acquire(dst->fdtab_lock);
	} else {
		slock_acquire(dst->fdtab_lock);
		slock_acquire(src->fdtab_lock);
	}

	/* Free the dst table */
	fd_tab_free_native(dst);

	/* Copy all valid descriptors */
	int x;
        for(x = 0;x < PROC_MAX_FDS;x++)
	{
		if(!src->fdtab[x]) continue;
		/* We created another ref to this fd */
		src->fdtab[x]->refs++;
		dst->fdtab[x] = src->fdtab[x];
	}

	/* Free both locks */
	slock_release(src->fdtab_lock);
	slock_release(dst->fdtab_lock);

	return 0;
}

int fd_new(struct proc* p, int index, int free)
{
	int result = 0;
	slock_acquire(p->fdtab_lock);
	if(p->fdtab[index]) 
	{
		result = 1;
		
	} else {
		p->fdtab[index] = fd_alloc();
		result = 0;
	}

	slock_release(p->fdtab_lock);
	return result;
}

void fd_print_table(void)
{
	cprintf("+------------------------------+\n");
	cprintf("|---------- FD TABLE ----------|\n");
	cprintf("+------------------------------+\n\n");

	int x;
	for(x = 0;x < FDS_TABLE_SZ;x++)
	{
		if(fds[x].type)
		{
			cprintf("%d: %s\n", x, fds[x].path);
			cprintf("\ttype:   ");
			switch(fds[x].type)
			{
				case FD_TYPE_INUSE:
					cprintf("LEAKED\n");
					break;
				case FD_TYPE_FILE:
					cprintf("FILE\n");
					break;
				case FD_TYPE_DEVICE:
					cprintf("DEVICE\n");
					break;
				case FD_TYPE_PIPE:
					cprintf("PIPE\n");
					break;
				case FD_TYPE_PATH:
					cprintf("PATH\n");
					break;
			}
			cprintf("\trefs:   %d\n", fds[x].refs);
			cprintf("\tflags:  %d\n", fds[x].flags);
			cprintf("\tseek:   %d\n", fds[x].seek);
			cprintf("\ti:      0x%x\n", fds[x].i);
			cprintf("\tdevice: 0x%x\n", fds[x].device);
			cprintf("\tpath:   %s\n", fds[x].path);
		}
	}
}
