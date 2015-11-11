#ifndef _TRAP_H_
#define _TRAP_H_

/* Include machine dependant header */
#ifdef ARCH_i386

#else
#error "Invalid architecture selected."
#endif

void push_cli(void);
void pop_cli(void);

#endif
