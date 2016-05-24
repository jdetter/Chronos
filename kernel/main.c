
#include <stdlib.h>
#include <string.h>

#include "kern/stdlib.h"
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
#include "idt.h"
#include "x86.h"
#include "cpu.h"
#include "ktime.h"
#include "trap.h"
#include "netman.h"
#include "signal.h"
#include "cacheman.h"
#include "fpu.h"
#include "drivers/pic.h"
#include "drivers/pit.h"
#include "drivers/cmos.h"
#include "drivers/rtc.h"
#include "drivers/keyboard.h"

void main_stack(void);

extern struct rtc_t k_time;
extern struct proc* rproc;
extern pstack_t k_stack;

/* Entry point for the kernel */
int main(void)
{
	arch_init(main_stack);

	panic("main_stack returned.\n");
	for(;;);
	return 0;
}

/* Proper 4K stack*/
void main_stack(void)
{
	/* We now have a proper kernel stack */
	cprintf("[ OK ]\n");

	/* Initilize signals */
	cprintf("Initilizing signals...\t\t\t\t\t\t\t");
	sig_init();
	cprintf("[ OK ]\n");
	
	/* Enable PIC */
        cprintf("Starting Programmable Interrupt Controller Driver...\t\t\t");
        pic_init();
        cprintf("[ OK ]\n");

	/* Enable the floating point unit */
	cprintf("Enabling the floating point unit.\t\t\t\t\t");
	fpu_init();
	cprintf("[ OK ]\n");

        /* Initilize CMOS */
        cprintf("Initilizing cmos...\t\t\t\t\t\t\t");
        cmos_init();
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
	cprintf("Detecting devices...\t\t\t\t\t\t\t");
	dev_init();
	cprintf("[ OK ]\n");

	/* Start file system manager */
	cprintf("Starting file system manager...\t\t\t\t\t\t");
	fsman_init();
	cprintf("[ OK ]\n");

	/* Populate the device folder */
	cprintf("Populating the dev folder...\t\t\t\t\t\t");
        dev_populate();
        cprintf("[ OK ]\n");

	cprintf("Starting network manager...\t\t\t\t\t\t");
	net_init();
	cprintf("[ OK ]\n");

	/* Enable PIT */
        cprintf("Starting Programmable Interrupt Timer Driver...\t\t\t\t");
	pit_init();
	cprintf("[ OK ]\n");

	/* Initilize cli stack */
	cprintf("Initilizing cli stack...\t\t\t\t\t\t");
	reset_cli();
	cprintf("[ OK ]\n");

	/* Initilize pipes */
	cprintf("Initilizing pipes...\t\t\t\t\t\t\t");
	pipe_init();
	cprintf("[ OK ]\n");
	
	cprintf("Initilizing Process Scheduler...\t\t\t\t\t");
	sched_init();
	cprintf("[ OK ]\n");

	cprintf("Initilizing kernel logger...\t\t\t\t\t\t");
	klog_init();
	cprintf("[ OK ]\n");

	cprintf("Starting all logs...\t\t\t\t\t\t\t");
        if(tty_code_log_init())
                cprintf("[FAIL]\n");
        else cprintf("[ OK ]\n");

	cprintf("Enabling logging on tty0...\t\t\t\t\t\t");
        if(tty_enable_logging(tty_find(0), "tty0.txt"))
                cprintf("[FAIL]\n");
        else cprintf("[ OK ]\n");
#if 0
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

	/* Spawn shells on all of the ttys */	
	cprintf("Spawning ttys...\t\t\t\t\t\t\t");
	int tty_num;
	for(tty_num = 1;;tty_num++)
	{
		if(tty_num == 2) break;
		tty_t t = tty_find(tty_num);
		spawn_tty(t);
		if(!t) break;
	}
	cprintf("[ OK ]\n");
	
	/* Start scheduling loop. */
	sched();
	panic("Scheduler returned!");
}

void msetup()
{
	panic("kernel: Memory allocator not initilized.\n");
}

int mpanic(int size)
{
	panic("kernel: Memory allocator out of space.\n");
	return -1;
}
