#ifndef _CPU_H_
#define _CPU_H_

/**
 * Push an interrupt request onto the cli stack. pop_cli must be called
 * before the function returns.
 */
void push_cli(void);

/**
 * Pops an interrupt request off of the cli stack. If the stack is empty,
 * interrupts are reenabled. Otherwise, interrupts remain disabled.
 */
void pop_cli(void);

/**
 * Empties the cli stack for the current cpu.
 */
void reset_cli(void);

/**
 * Shutdown the system
 */
void cpu_shutdown(void);

/**
 * Reboot the system
 */
void cpu_reboot(void);

#endif
