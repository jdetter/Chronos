#ifndef _PANIC_H_
#define _PANIC_H_

/**
 * Initilize the primary tty for the first time.
 */
void cprintf_init(void);

/**
 * Hacky version of printf that doesn't require va_args. This method
 * is dangerous if not called properly. This method can be called any time
 * once the kernel has been loaded.
 */
void cprintf(char* fmt, ...);
void panic(char* fmt, ...) __attribute__ ((noreturn));

#endif
