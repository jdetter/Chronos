#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "k/drivers/console.h"
#include "x86.h"

void cprintf_init(void)
{
        tty_t t0 = tty_find(0);
        if(t0->type == 0)
        {
                video_mode = VIDEO_MODE_NONE;
                // video_mode = VIDEO_MODE_COLOR;
                switch(video_mode)
                {
                        case VIDEO_MODE_NONE:
                        default:
                                tty_init(t0, 0, TTY_TYPE_SERIAL, 0, 0);
                                break;
                        case VIDEO_MODE_COLOR:
                                tty_init(t0, 0, TTY_TYPE_COLOR, 1,
                                        (uint)CONSOLE_COLOR_BASE_ORIG);
                                break;
                        case VIDEO_MODE_MONO:
                                tty_init(t0, 0, TTY_TYPE_MONO, 1,
                                        (uint)CONSOLE_MONO_BASE_ORIG);
                                break;
                }
                tty_enable(t0);
        }
}
