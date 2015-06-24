#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "stdarg.h"
#include "stdlib.h"
#include "console.h"

/**
 * Hacky version of printf that doesn't require va_args. This method
 * is dangerous if not called properly. This method can be called any time
 * once the kernel has been loaded.
 */
void cprintf(char* fmt, ...)
{
	tty_t t0 = tty_find(0);
        if(t0->type == 0)
        {
                tty_init(t0, 0, TTY_TYPE_SERIAL, 0, 0);
                //tty_init(t0, 0, TTY_TYPE_COLOR, 1, 
                //     (uint)CONSOLE_COLOR_BASE_ORIG);
                tty_enable(t0);
        }

        void** argument = (void**)(&fmt + 1);

        uint x;
        for(x = 0;x < strlen(fmt);x++)
        {
                if(fmt[x] == '%' && x + 1 < strlen(fmt))
                {
                        if(fmt[x + 1] == '%')
                                tty_print_character(t0, '%');
                        else if(fmt[x + 1] == 'd')
                        {
                                /* Print in decimal */
                                char buffer[32];
                                itoa(*((int*)argument), buffer, 32, 10);
                                uint y;
                                for(y = 0;y < strlen(buffer);y++)
                                        tty_print_character(t0, buffer[y]);
                                argument++;
                        } else if(fmt[x + 1] == 'p' || fmt[x + 1] == 'x')
                        {
                                /* Print in hex */
                                char buffer[32];
                                itoa(*((int*)argument), buffer, 32, 16);
                                uint y;
                                for(y = 0;y < strlen(buffer);y++)
                                        tty_print_character(t0, buffer[y]);
                                argument++;
                        } else if(fmt[x + 1] == 'c')
                        {
                                /* Print character */
                                char c = *((char*)argument);
                                tty_print_character(t0, c);
                                argument++;
                        } else if(fmt[x + 1] == 's')
                        {
                                char* str = *((char**)argument);
                                int y;
                                for(y = 0;y < strlen(str);y++)
                                        tty_print_character(t0, str[y]);
                                argument++;
                        } else if(fmt[x + 1] == 'b')
                        {
                                /* Print in binary */
                                char buffer[128];
                                itoa(*((int*)argument), buffer, 32, 2);
                                uint y;
                                for(y = 0;y < strlen(buffer);y++)
                                        tty_print_character(t0, buffer[y]);
                                argument++;
                        }

                        x++;
                } else tty_print_character(t0, fmt[x]);
	}
}

void panic(char* fmt, ...)
{
	asm volatile("addl $0x08, %esp");
	asm volatile("call cprintf");
	for(;;);
}
