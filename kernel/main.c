
#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "file.h"
#include "stdarg.h"
#include "stdlock.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "cpu.h"
#include "ktime.h"
#include "trap.h"
#include "signal.h"
#include "cacheman.h"

void main_stack(void);

extern pstack_t k_stack;

extern void arch_init(void (*function)(void));

/* Entry point for the kernel */
int main(void)
{
	/* First copy over the root partition argument */
	// strncpy(root_partition, root_part, FILE_MAX_PATH - 1);
	arch_init(main_stack);

	panic("main_stack returned.\n");
	for(;;);
	return 0;
}

/* Proper minimum 4K stack*/
void main_stack(void)
{
	/* We now have a proper kernel stack */
	cprintf("[ OK ]\n");

	/* Initilize signals */
	cprintf("Initilizing signals...\t\t\t\t\t\t\t");
	sig_init();
	cprintf("[ OK ]\n");

	/* Initilize kernel time */
	cprintf("Initilizing kernel time...\t\t\t\t\t\t");
	ktime_init();
	cprintf("[ OK ]\n");

	/* Initilize caches */
	cprintf("Initilizing caches...\t\t\t\t\t\t\t");
	cman_init();
	cprintf("[ OK ]\n");

	/* Detect devices */
	cprintf("Initilizing device manager...\n");
	devman_init();

	/* Initilize pipes */
	cprintf("Initilizing pipes...\t\t\t\t\t\t\t");
	pipe_init();
	cprintf("[ OK ]\n");

	/* Start file system manager */
	cprintf("Starting file system manager...\t\t\t\t\t\t");
	fsman_init();
	cprintf("[ OK ]\n");

	/* Make sure all of the log folders exist and are setup */
	cprintf("Initilizing kernel logger...\t\t\t\t\t\t");
	klog_init();
	cprintf("[ OK ]\n");

	/* Populate the device folder */
	cprintf("Populating the dev folder...\t\t\t\t\t\t");
	dev_populate();
	cprintf("[ OK ]\n");

	cprintf("Initilizing Process Scheduler...\t\t\t\t\t");
	sched_init();
	cprintf("[ OK ]\n");

	cprintf("Starting all logs...\t\t\t\t\t\t\t");
	if(tty_code_log_init())
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");

#if 0
	cprintf("Enabling logging on tty0...\t\t\t\t\t\t");
	if(tty_enable_logging(tty_find(0), "tty0.txt"))
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");
	cprintf("Enabling logging on tty0...\t\t\t\t\t\t");
	if(tty_enable_logging(tty_find(0), "tty0-new.txt"))
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");

	cprintf("Enabling logging on tty1...\t\t\t\t\t\t");
	if(tty_enable_logging(tty_find(1), "tty1-new.txt"))
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");

	cprintf("Starting all logs...\t\t\t\t\t\t\t");
	if(tty_code_log_init())
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");

	cprintf("Enabling logging on tty1...\t\t\t\t\t\t");
	if(tty_enable_logging(tty_find(1), "tty1.txt"))
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");

	cprintf("Enabling console code logging on tty1...\t\t\t\t");
	if(tty_enable_code_logging(tty_find(1)))
		cprintf("[FAIL]\n");
	else cprintf("[ OK ]\n");
#endif

	cprintf("Spawning primary tty...\t\t\t\t\t\t\t");
	tty_t primary = tty_find(0);
	if(!primary)
		panic("[FAIL]\nNo primary tty found!\n");
	tty_spawn(primary);
	// tty_enable(primary);
	cprintf("[ OK ]\n");

#if 0
	/* Spawn shells on all of the ttys */	
	cprintf("Spawning ttys...\t\t\t\t\t\t\t");
	int tty_num;
	for(tty_num = 0;tty_num < 4;tty_num++)
	{
		if(tty_num == 1) break;
		tty_t t = tty_find(tty_num);
		tty_spawn(t);
		if(!t) break;
	}
	cprintf("[ OK ]\n");
#endif

	/* Start scheduling loop. */
	sched();
	panic("Scheduler returned!");
}
