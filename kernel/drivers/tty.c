#include <string.h>

#include "tty.h"
#include "stdlock.h"
#include "proc.h"
#include "devman.h"
#include "panic.h"
#include "elf.h"
#include "drivers/console.h"

static struct tty ttys[MAX_TTYS];
struct tty* active_tty = NULL;

tty_t tty_find(int index)
{
	    if(index >= MAX_TTYS) return NULL;
		    return ttys + index;
}

int tty_num(tty_t t)
{
    if((char*)t >= (char*)ttys && (char*)t < (char*)(ttys + MAX_TTYS))
        return t->num;
    else panic("Invalid tty given to tty_num\n");
}

int tty_enable_code_logging(tty_t t)
{
    t->codes_logged = 1;
    return 0;
}

int tty_enable_logging(tty_t t, char* file)
{
    t->tty_log = klog_alloc(0, file);

    /* Did it get opened? */
    if(!t->tty_log)
        return -1;

    t->out_logged = 1;

    return 0;
}

void tty_enable(tty_t t)
{
    /* unset the currently active tty */
    if(active_tty)
        active_tty->active = 0;

    t->active = 1;
    active_tty = t;

    if(t->type==TTY_TYPE_SERIAL)
    {
        return;
    }

    if(t->type == TTY_TYPE_MONO || t->type == TTY_TYPE_COLOR)
    {
        tty_print_screen(t, t->buffer);
        console_update_cursor(t->cursor_pos);
    }
}

tty_t tty_active(void)
{
    return active_tty;
}

void tty_disable(tty_t t)
{
    t->active=0;
}

tty_t tty_check(struct IODevice* driver)
{
    uintptr_t addr = (uintptr_t)driver->context;
    if(addr >= (uintptr_t)ttys && addr < (uintptr_t)ttys + MAX_TTYS)
    {
        /**
         * addr points into the table so this is most
         * likely a tty.
         */
        return (tty_t)addr;
    }

    return NULL;
}

void tty_reset_sgr(tty_t t);
void tty_init(tty_t t, int num, char type, int cursor_enabled,
        uintptr_t mem_start)
{
    memset(t, 0, sizeof(tty_t));

    t->num = num;
    t->active = 0; /* 1: This tty is in the foreground, 0: background*/
    t->type = type;
    t->tab_stop = 8;
    tty_reset_sgr(t); /* Reset the display properties */
    t->saved = 0;
    int i;
    for(i = 0; i<TTY_BUFFER_SZ; i+=2)/*sets text and graphics buffers*/
    {
        t->buffer[i]= ' ';
        t->buffer[i+1]= t->sgr.color;
    }
    t->cursor_pos = 0; /* Text mode position of the cursor. */
    t->cursor_enabled= cursor_enabled; /* Whether or not to show the cursor (text)*/
    t->mem_start = mem_start; /* The start of video memory. (color, mono only)*/

    /* Init keyboard lock */
    slock_init(&t->key_lock);

    /* Set the window spec */
    t->window.ws_row = CONSOLE_ROWS;
    t->window.ws_col = CONSOLE_COLS;

    /* Set the behavior */
    t->term.c_iflag = IGNBRK | IGNPAR | ICRNL;
    t->term.c_oflag = OCRNL | ONLRET;
    t->term.c_cflag = 0;
    t->term.c_lflag = ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL;

    /* TODO: handle control characters */
    memset(t->term.c_cc, 0, NCCS);
}

int tty_connected(tty_t t)
{
	slock_acquire(&ptable_lock);
	int result = 0;
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].t == t)
		{
			result = 1;
			break;
		}
	}
	slock_release(&ptable_lock);
	return result;
}

void tty_disconnect_proc(struct proc* p)
{
	slock_acquire(&ptable_lock);
	/* disconnect stdin, stdout and stderr */
	fd_free(p, 0);
	fd_free(p, 1);
	fd_free(p, 2);

	tty_t t = p->t;

	/* remove controlling terminal */
	p->t = NULL;
	slock_release(&ptable_lock);

	if(p->sid == p->sid)
		tty_disconnect_all(t);
}

void tty_disconnect_all(tty_t t)
{
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].t == t)
			tty_disconnect_proc(ptable + x);
	}
}

void tty_set_proc_ctty(struct proc* p, tty_t t)
{
	slock_acquire(&ptable_lock);
	rproc->t = t;
	if(fd_new(p, 0, 1) == -1) return;
	if(fd_new(p, 1, 1) == -1) return;
	if(fd_new(p, 2, 1) == -1) return;

	rproc->fdtab[0]->device = t->driver;
	rproc->fdtab[0]->type = FD_TYPE_DEVICE;
	rproc->fdtab[1]->device = t->driver;
	rproc->fdtab[1]->type = FD_TYPE_DEVICE;
	rproc->fdtab[2]->device = t->driver;
	rproc->fdtab[2]->type = FD_TYPE_DEVICE;
	slock_release(&ptable_lock);
}

int tty_spawn(tty_t t)
{
	/* Try to get a new process */
	struct proc* p = alloc_proc();
	if(!p) return -1; /* Could we find an unused process? */

	/* Get the process table lock */
	slock_acquire(&ptable_lock);

	/* Setup the new process */
	p->t = t;
	p->pid = next_pid++;
	p->uid = 0; /* init is owned by root */
	p->gid = 0; /* group is also root */

	/* Setup stdin, stdout and stderr */
	if(fd_next(p) != 0) panic("spawn_tty: wrong fd for stdin\n");
	p->fdtab[0]->type = FD_TYPE_DEVICE;
	p->fdtab[0]->device = t->driver;
	if(fd_next(p) != 1) panic("spawn_tty: wrong fd for stdout\n");
	p->fdtab[1]->type = FD_TYPE_DEVICE;
	p->fdtab[1]->device = t->driver;
	if(fd_next(p) != 2) panic("spawn_tty: wrong fd for stderr\n");
	p->fdtab[2]->type = FD_TYPE_DEVICE;
	p->fdtab[2]->device = t->driver;

	p->stack_start = PGROUNDUP(UVM_TOP);
	p->stack_end = p->stack_start - PGSIZE;

	p->block_type = PROC_BLOCKED_NONE;
	p->b_condition = NULL;
	p->b_pid = 0;
	p->parent = p;

	strncpy(p->name, "init", MAX_PROC_NAME);
	strncpy(p->cwd, "/", MAX_PATH_LEN);

	/* Setup virtual memory */
	p->pgdir = (pgdir_t*)palloc();
	vm_copy_kvm(p->pgdir);

	vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT;

	/* Map in a new kernel stack */
	vm_mappages(UVM_KSTACK_S, UVM_KSTACK_E - UVM_KSTACK_S, p->pgdir,
			dir_flags, tbl_flags);
	p->k_stack = (context_t)PGROUNDUP(UVM_KSTACK_E);
	p->tf = (struct trap_frame*)(p->k_stack - sizeof(struct trap_frame));
	p->tss = (struct task_segment*)(UVM_KSTACK_S);

	/* Map in a user stack. */
	vm_mappages(p->stack_end, PGSIZE, p->pgdir,
			dir_flags | VM_DIR_USRP, tbl_flags | VM_TBL_USRP);

	/* Switch to user page table */
	vm_enable_paging(p->pgdir);

	/* Basic values for the trap frame */
	/* Setup the user stack */
	char* ustack = (char*)p->stack_start;

	/* Fake env */
	ustack -= sizeof(int);
	*((uintptr_t*)ustack) = 0x0;

	/* Fake argv */
	ustack -= sizeof(int);
	*((uintptr_t*)ustack) = 0x0;

	/* argc = 0 */
	ustack -= sizeof(int);
	*((uintptr_t*)ustack) = 0x0;

	/* Fake eip */
	ustack -= sizeof(int);
	*((uintptr_t*)ustack) = 0xFFFFFFFF;
	p->tf->esp = (uintptr_t)ustack;

	/* Load the binary */
	uintptr_t code_start;
	uintptr_t code_end;
	uintptr_t entry = elf_load_binary_path("/bin/init", p->pgdir,
			&code_start, &code_end, 1);
	p->code_start = code_start;
	p->code_end = code_end;
	p->entry_point = entry;
	p->heap_start = PGROUNDUP(code_end);
	p->heap_end = p->heap_start;

	/* Set the mmap area start */
	p->mmap_end = p->mmap_start =
		PGROUNDUP(UVM_TOP) - UVM_MIN_STACK;

	p->state = PROC_READY;
	slock_release(&ptable_lock);

	/* Return Success */
	return 0;
}
