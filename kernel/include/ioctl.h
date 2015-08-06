#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

/** Start Ubuntu Linux /usr/include/bits/ioctl-types.h */
/* LICENSE COPIED DIRECTLY FROM SOURCE */

/* Structure types for pre-termios terminal ioctls.  Linux version.
   Copyright (C) 1996-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

// #include <asm/ioctls.h> Included below - no deps required

struct winsize
  {
    unsigned short int ws_row;
    unsigned short int ws_col;
    unsigned short int ws_xpixel;
    unsigned short int ws_ypixel;
  };

#define NCCS 32
struct termio
  {
    unsigned short int c_iflag;         /* input mode flags */
    unsigned short int c_oflag;         /* output mode flags */
    unsigned short int c_cflag;         /* control mode flags */
    unsigned short int c_lflag;         /* local mode flags */
    unsigned char c_line;               /* line discipline */
    unsigned char c_cc[NCCS];            /* control characters */
};

/* modem lines */
#define TIOCM_LE        0x001
#define TIOCM_DTR       0x002
#define TIOCM_RTS       0x004
#define TIOCM_ST        0x008
#define TIOCM_SR        0x010
#define TIOCM_CTS       0x020
#define TIOCM_CAR       0x040
#define TIOCM_RNG       0x080
#define TIOCM_DSR       0x100
#define TIOCM_CD        TIOCM_CAR
#define TIOCM_RI        TIOCM_RNG

/* ioctl (fd, TIOCSERGETLSR, &result) where result may be as below */

/* line disciplines */
#define N_TTY           0
#define N_SLIP          1
#define N_MOUSE         2
#define N_PPP           3
#define N_STRIP         4
#define N_AX25          5
#define N_X25           6       /* X.25 async  */
#define N_6PACK         7
#define N_MASC          8       /* Mobitex module  */
#define N_R3964         9       /* Simatic R3964 module  */
#define N_PROFIBUS_FDL  10      /* Profibus  */
#define N_IRDA          11      /* Linux IR  */
#define N_SMSBLOCK      12      /* SMS block mode  */
#define N_HDLC          13      /* synchronous HDLC  */
#define N_SYNC_PPP      14      /* synchronous PPP  */
#define N_HCI           15      /* Bluetooth HCI UART  */


/** End Ubuntu Linux /usr/include/bits/ioctl-types.h */

/** Start Ubuntu Linux /usr/include/asm-generic/ioctls.h */

/* ioctl command encoding: 32 bits total, command in lower 16 bits,
 * size of the parameter structure in the lower 14 bits of the
 * upper 16 bits.
 * Encoding the size of the parameter structure in the ioctl request
 * is useful for catching programs compiled with old versions
 * and to avoid overwriting user space outside the user buffer area.
 * The highest 2 bits are reserved for indicating the ``access mode''.
 * NOTE: This limits the max parameter size to 16kB -1 !
 */

/*
 * The following is for compatibility across the various Linux
 * platforms.  The generic ioctl numbering scheme doesn't really enforce
 * a type field.  De facto, however, the top 8 bits of the lower 16
 * bits are indeed used as a type field, so we might just as well make
 * this explicit here.  Please be sure to use the decoding macros
 * below from now on.
 */
#define _IOC_NRBITS     8
#define _IOC_TYPEBITS   8

/*
 * Let any architecture override either of the following before
 * including this file.
 */

#ifndef _IOC_SIZEBITS
# define _IOC_SIZEBITS  14
#endif

#ifndef _IOC_DIRBITS
# define _IOC_DIRBITS   2
#endif

#define _IOC_NRMASK     ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK   ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK   ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK    ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT+_IOC_SIZEBITS)

/*
 * Direction bits, which any architecture can choose to override
 * before including this file.
 */

#ifndef _IOC_NONE
# define _IOC_NONE      0U
#endif

#ifndef _IOC_WRITE
# define _IOC_WRITE     1U
#endif

#ifndef _IOC_READ
# define _IOC_READ      2U
#endif

#define _IOC(dir,type,nr,size) \
        (((dir)  << _IOC_DIRSHIFT) | \
         ((type) << _IOC_TYPESHIFT) | \
         ((nr)   << _IOC_NRSHIFT) | \
         ((size) << _IOC_SIZESHIFT))

#define _IOC_TYPECHECK(t) (sizeof(t))

/* used to create numbers */
#define _IO(type,nr)            _IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)      _IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size)      _IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size)     _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOR_BAD(type,nr,size)  _IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW_BAD(type,nr,size)  _IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _IOWR_BAD(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))

/* used to decode ioctl numbers.. */
#define _IOC_DIR(nr)            (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)           (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)             (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)           (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

/* ...and for the drivers/sound files... */

#define IOC_IN          (_IOC_WRITE << _IOC_DIRSHIFT)
#define IOC_OUT         (_IOC_READ << _IOC_DIRSHIFT)
#define IOC_INOUT       ((_IOC_WRITE|_IOC_READ) << _IOC_DIRSHIFT)
#define IOCSIZE_MASK    (_IOC_SIZEMASK << _IOC_SIZESHIFT)
#define IOCSIZE_SHIFT   (_IOC_SIZESHIFT)

/** End Ubuntu Linux /usr/include/asm-generic/ioctls.h */

/** Start Ubuntu Linux /usr/include/asm-generic/ioctls.h */

/*
 * These are the most common definitions for tty ioctl numbers.
 * Most of them do not use the recommended _IOC(), but there is
 * probably some source code out there hardcoding the number,
 * so we might as well use them for all new platforms.
 *
 * The architectures that use different values here typically
 * try to be compatible with some Unix variants for the same
 * architecture.
 */

/* 0x54 is just a magic number to make these relatively unique ('T') */

#define TCGETS          0x5401
#define TCSETS          0x5402
#define TCSETSW         0x5403
#define TCSETSF         0x5404
#define TCGETA          0x5405
#define TCSETA          0x5406
#define TCSETAW         0x5407
#define TCSETAF         0x5408
#define TCSBRK          0x5409
#define TCXONC          0x540A
#define TCFLSH          0x540B
#define TIOCEXCL        0x540C
#define TIOCNXCL        0x540D
#define TIOCSCTTY       0x540E
#define TIOCGPGRP       0x540F
#define TIOCSPGRP       0x5410
#define TIOCOUTQ        0x5411
#define TIOCSTI         0x5412
#define TIOCGWINSZ      0x5413
#define TIOCSWINSZ      0x5414
#define TIOCMGET        0x5415
#define TIOCMBIS        0x5416
#define TIOCMBIC        0x5417
#define TIOCMSET        0x5418
#define TIOCGSOFTCAR    0x5419
#define TIOCSSOFTCAR    0x541A
#define FIONREAD        0x541B
#define TIOCINQ         FIONREAD
#define TIOCLINUX       0x541C
#define TIOCCONS        0x541D
#define TIOCGSERIAL     0x541E
#define TIOCSSERIAL     0x541F
#define TIOCPKT         0x5420
#define FIONBIO         0x5421
#define TIOCNOTTY       0x5422
#define TIOCSETD        0x5423
#define TIOCGETD        0x5424
#define TCSBRKP         0x5425  /* Needed for POSIX tcsendbreak() */
#define TIOCSBRK        0x5427  /* BSD compatibility */
#define TIOCCBRK        0x5428  /* BSD compatibility */
#define TIOCGSID        0x5429  /* Return the session ID of FD */
#define TCGETS2         _IOR('T', 0x2A, struct termios2)
#define TCSETS2         _IOW('T', 0x2B, struct termios2)
#define TCSETSW2        _IOW('T', 0x2C, struct termios2)
#define TCSETSF2        _IOW('T', 0x2D, struct termios2)
#define TIOCGRS485      0x542E
#ifndef TIOCSRS485
#define TIOCSRS485      0x542F
#endif
#define TIOCGPTN        _IOR('T', 0x30, unsigned int) /* Get Pty Number (of pty-mux device) */
#define TIOCSPTLCK      _IOW('T', 0x31, int)  /* Lock/unlock Pty */
#define TIOCGDEV        _IOR('T', 0x32, unsigned int) /* Get primary device node of /dev/console */
#define TCGETX          0x5432 /* SYS5 TCGETX compatibility */
#define TCSETX          0x5433
#define TCSETXF         0x5434
#define TCSETXW         0x5435
#define TIOCSIG         _IOW('T', 0x36, int)  /* pty: generate signal */
#define TIOCVHANGUP     0x5437
#define TIOCGPKT        _IOR('T', 0x38, int) /* Get packet mode state */
#define TIOCGPTLCK      _IOR('T', 0x39, int) /* Get Pty lock state */
#define TIOCGEXCL       _IOR('T', 0x40, int) /* Get exclusive mode state */

#define FIONCLEX        0x5450
#define FIOCLEX         0x5451
#define FIOASYNC        0x5452
#define TIOCSERCONFIG   0x5453
#define TIOCSERGWILD    0x5454
#define TIOCSERSWILD    0x5455
#define TIOCGLCKTRMIOS  0x5456
#define TIOCSLCKTRMIOS  0x5457
#define TIOCSERGSTRUCT  0x5458 /* For debugging only */
#define TIOCSERGETLSR   0x5459 /* Get line status register */
#define TIOCSERGETMULTI 0x545A /* Get multiport config  */
#define TIOCSERSETMULTI 0x545B /* Set multiport config */

#define TIOCMIWAIT      0x545C  /* wait for a change on serial input line(s) */
#define TIOCGICOUNT     0x545D  /* read serial port __inline__ interrupt counts */

/*
 * Some arches already define FIOQSIZE due to a historical
 * conflict with a Hayes modem-specific ioctl value.
 */
#ifndef FIOQSIZE
# define FIOQSIZE       0x5460
#endif

/* Used for packet mode */
#define TIOCPKT_DATA             0
#define TIOCPKT_FLUSHREAD        1
#define TIOCPKT_FLUSHWRITE       2
#define TIOCPKT_STOP             4
#define TIOCPKT_START            8
#define TIOCPKT_NOSTOP          16
#define TIOCPKT_DOSTOP          32
#define TIOCPKT_IOCTL           64

#define TIOCSER_TEMT    0x01    /* Transmitter physically empty */

/** End Ubuntu Linux /usr/include/asm-generic/ioctls.h */

typedef unsigned char cc_t; 
typedef unsigned int  speed_t; 
typedef unsigned int  tcflag_t;

struct termios
{
	tcflag_t c_iflag; /* input mode flags */
	tcflag_t c_oflag; /* output mode flags */
	tcflag_t c_cflag; /* control mode flags */
	tcflag_t c_lflag; /* local mode flags */
	cc_t c_line; 		/* line disipline */
	cc_t c_cc[NCCS]; /* control characters */
	speed_t c_ispeed; /* input speed */
	speed_t c_ospeed; /* output speed */
#define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
#define _HAVE_STRUCT_TERMIOS_C_OSPEED
};

/* c_cc characters */
/**
 * (003, ETX, Ctrl-C, or also 0177, DEL, rubout) Interrupt
 * character (INTR).  Send a SIGINT signal.  Recognized when ISIG
 * is set, and then not passed as input.
 */
#define VINTR 0

/**
 * (034, FS, Ctrl-\) Quit character (QUIT).  Send SIGQUIT signal.
 * Recognized when ISIG is set, and then not passed as input.
 */
#define VQUIT 1

/**
 * (0177, DEL, rubout, or 010, BS, Ctrl-H, or also #) Erase
 * character (ERASE).  This erases the previous not-yet-erased
 * character, but does not erase past EOF or beginning-of-line.
 * Recognized when ICANON is set, and then not passed as input.
 */
#define VERASE 2

/**
 * (025, NAK, Ctrl-U, or Ctrl-X, or also @) Kill character
 * (KILL).  This erases the input since the last EOF or
 * beginning-of-line.  Recognized when ICANON is set, and then
 * not passed as input.
 */
#define VKILL 3

/**
 * (004, EOT, Ctrl-D) End-of-file character (EOF).  More
 * precisely: this character causes the pending tty buffer to be
 * sent to the waiting user program without waiting for end-of-
 * line.  If it is the first character of the line, the read(2)
 * in the user program returns 0, which signifies end-of-file.
 * Recognized when ICANON is set, and then not passed as input.
 */
#define VEOF 4

/**
 * Timeout in deciseconds for noncanonical read (TIME).
 */
#define VTIME 5

/**
 * Minimum number of characters for noncanonical read (MIN).
 */
#define VMIN 6

/**
 * No linux support? 
 */
#define VSWTC 7

/**
 *(021, DC1, Ctrl-Q) Start character (START).  Restarts output
 * stopped by the Stop character.  Recognized when IXON is set,
 * and then not passed as input.
 */
#define VSTART 8

/**
 * (023, DC3, Ctrl-S) Stop character (STOP).  Stop output until
 * Start character typed.  Recognized when IXON is set, and then
 * not passed as input.
 */
#define VSTOP 9

/**
 * (032, SUB, Ctrl-Z) Suspend character (SUSP).  Send SIGTSTP
 * signal.  Recognized when ISIG is set, and then not passed as
 * input.
 */
#define VSUSP 10

/**
 * Yet another end-of-line character
 * (EOL2).  Recognized when ICANON is set.
 */
#define VEOL 11

/**
 * (not in POSIX; 022, DC2, Ctrl-R) Reprint unread characters
 * (REPRINT).  Recognized when ICANON and IEXTEN are set, and
 * then not passed as input.
 */
#define VREPRINT 12

/**
 * (not in POSIX; not supported under Linux; 017, SI, Ctrl-O)
 * Toggle: start/stop discarding pending output.  Recognized when
 * IEXTEN is set, and then not passed as input.
 */
#define VDISCARD 13

/**
 * (not in POSIX; 027, ETB, Ctrl-W) Word erase (WERASE).
 * Recognized when ICANON and IEXTEN are set, and then not passed
 * as input.
 */
#define VWERASE 14

/**
 * (not in POSIX; 026, SYN, Ctrl-V) Literal next (LNEXT).  Quotes
 * the next input character, depriving it of a possible special
 * meaning.  Recognized when IEXTEN is set, and then not passed
 * as input.
 */
#define VLNEXT 15

/**
 * (not in POSIX; 0, NUL) Yet another end-of-line character
 * (EOL2).  Recognized when ICANON is set.
 */
#define VEOL2 16

/* c_iflag bits */
#define IGNBRK  0000001 /* Ignore BREAK condition on input */

/**
 * If IGNBRK is set, a BREAK is ignored.  If it is not set but
 * BRKINT is set, then a BREAK causes the input and output queues
 * to be flushed, and if the terminal is the controlling terminal
 * of a foreground process group, it will cause a SIGINT to be
 * sent to this foreground process group.  When neither IGNBRK
 * nor BRKINT are set, a BREAK reads as a null byte ('\0'),
 * except when PARMRK is set, in which case it reads as the
 * sequence \377 \0 \0.
 */
#define BRKINT  0000002

#define IGNPAR  0000004 /* Ignore framing and parity errors */

/**
 * If IGNPAR is not set, prefix a character with a parity error
 * or framing error with \377 \0.  If neither IGNPAR nor PARMRK
 * is set, read a character with a parity error or framing error
 * as \0.
 */
#define PARMRK  0000010
#define INPCK   0000020 /* Enable input parity checking */
#define ISTRIP  0000040 /* strip off the 8th bit */
#define INLCR   0000100 /* translate NL to CR on input */
#define IGNCR   0000200 /* Ignore carrige return on input */
#define ICRNL   0000400 /* Translate carrige return to new line on input */
#define IUCLC   0001000 /* Map uppercase characters to lowercase on input */
#define IXON    0002000 /* Enable XON/XOFF flow control on output. */

/**
 * Typing any character will restart stopped output.
 * Default: allow just eht START character to restart output
 */
#define IXANY   0004000 

/**
 * Enable XON/XOFF flow control on input 
 */
#define IXOFF   0010000

/**
 * Ring bell when input queue is full.
 */
#define IMAXBEL 0020000
/** 
 * Input is UTF8; this allows character-erase to be correctly performed in 
 * cooked mode.*/
#define IUTF8   0040000 /* Input is UTF8; this allows character-erase to be correctly performed in cooked mode.*/

/* c_oflag bits */
#define OPOST   0000001 /* Enable output processing */
#define OLCUC   0000002 /* Map lowercase characters to uppercase on output */
#define ONLCR   0000004 /* XSI Map NL to CR-NL on output */
#define OCRNL   0000010 /* MAP CR to NL on output */
#define ONOCR   0000020 /* Don't output CR at column 0 */
#define ONLRET  0000040 /* Don't output CR */
#define OFILL   0000100 /* Send fill characters for a delay */

/**
 * Fill character is ASCII DEL (0177). If unset, the fill
 * character is ASCII NUL ('\0'). (no implement)
 */
#define OFDEL   0000200 

# define NLDLY  0000400 /* Newline delay mask */
# define   NL0  0000000
# define   NL1  0000400
# define CRDLY  0003000 /* Carrige return delay mask */
# define   CR0  0000000
# define   CR1  0001000
# define   CR2  0002000
# define   CR3  0003000
# define TABDLY 0014000 /* Horizontal tab delay mask */
# define   TAB0 0000000
# define   TAB1 0004000
# define   TAB2 0010000
# define   TAB3 0014000
# define BSDLY  0020000 /* Backspace delay mask. */
# define   BS0  0000000
# define   BS1  0020000
# define FFDLY  0100000 /* ??? */
# define   FF0  0000000
# define   FF1  0100000

#define VTDLY   0040000 /* Vertical tab delay mask */
#define   VT0   0000000
#define   VT1   0040000

#ifdef __USE_MISC
# define XTABS  0014000 /* ??? */
#endif

/* c_cflag bit meaning */
#ifdef __USE_MISC
# define CBAUD  0010017 /* Baud speed mask */
#endif
#define  B0     0000000         /* hang up */
#define  B50    0000001
#define  B75    0000002
#define  B110   0000003
#define  B134   0000004
#define  B150   0000005
#define  B200   0000006
#define  B300   0000007
#define  B600   0000010
#define  B1200  0000011
#define  B1800  0000012
#define  B2400  0000013
#define  B4800  0000014
#define  B9600  0000015
#define  B19200 0000016
#define  B38400 0000017
#ifdef __USE_MISC
# define EXTA B19200
# define EXTB B38400
#endif
#define CSIZE   0000060 /* Character size mask */
#define   CS5   0000000
#define   CS6   0000020
#define   CS7   0000040
#define   CS8   0000060
#define CSTOPB  0000100 /* Set 2 stop bits rather than one */
#define CREAD   0000200 /* Enable reciever */
#define PARENB  0000400 /* Enable parity generation for both io */
#define PARODD  0001000 /* If set, parity is odd. Otherwise parity is even.*/
#define HUPCL   0002000 /* last process causes hang up. */
#define CLOCAL  0004000 /* ignore modem control lines. */
# define CBAUDEX 0010000 /* Extra baud speed mask */
#define  B57600   0010001
#define  B115200  0010002
#define  B230400  0010003
#define  B460800  0010004
#define  B500000  0010005
#define  B576000  0010006
#define  B921600  0010007
#define  B1000000 0010010
#define  B1152000 0010011
#define  B1500000 0010012
#define  B2000000 0010013
#define  B2500000 0010014
#define  B3000000 0010015
#define  B3500000 0010016
#define  B4000000 0010017
#define __MAX_BAUD B4000000
# define CIBAUD   002003600000          /* input baud rate (not used) */
# define CMSPAR   010000000000          /* mark or space (stick) parity */
# define CRTSCTS  020000000000          /* flow control */

/* c_lflag bits */
#define ISIG    0000001 /* Generate signals for INTR, QUIT, SUSP and DSUSP*/
#define ICANON  0000002 /* Enable canonical mode */
# define XCASE  0000004 /* If ICANNON is set, terminal is upperase only */
#define ECHO    0000010 /* Echo input characters. */

/**
 * If ICANON is also set, the ERASE character erases the
 * preceding input character, and WERASE erases the preceding
 * word.
 */
#define ECHOE   0000020

/**
 * If ICANON is also set, the KILL character erases the current
 * line.
 */
#define ECHOK   0000040

/**
 * If ICANON is also set, echo the NL character even if ECHO is
 * not set.
 */
#define ECHONL  0000100

/**
 * Disable flushing the input and output queues when generating
 * signals for the INT, QUIT, and SUSP characters.
 */
#define NOFLSH  0000200

/**
 * Send the SIGTTOU signal to the process group of a background
 * process which tries to write to its controlling terminal.
 */
#define TOSTOP  0000400

/**
 * If ECHO is also set, terminal special
 * characters other than TAB, NL, START, and STOP are echoed as
 * ^X, where X is the character with ASCII code 0x40 greater than
 * the special character.  For example, character 0x08 (BS) is
 * echoed as ^H.  [requires _BSD_SOURCE or _SVID_SOURCE]
 */
# define ECHOCTL 0001000

/**
 * If ICANON and ECHO are also set, characters are
 * printed as they are being erased.
 */
# define ECHOPRT 0002000

/**
 * If ICANON is also set, KILL is echoed by
 * erasing each character on the line, as specified by ECHOE and
 * ECHOPRT
 */
# define ECHOKE  0004000

/**
 * Output is being flushed. This flag is toggled by typing the DISCARD
 * character.
 */
# define FLUSHO  0010000

/**
 * All characters in
 * the input queue are reprinted when the next character is read.
 */
# define PENDIN  0040000
/**
 * Enable implementation-defined input processing.  This flag, as
 * well as ICANON must be enabled for the special characters
 * EOL2, LNEXT, REPRINT, WERASE to be interpreted, and for the
 * IUCLC flag to be effective.
 */
#define IEXTEN  0100000
#ifdef __USE_BSD
# define EXTPROC 0200000 /** ??? */
#endif

/* tcflow() and TCXONC use these */
#define TCOOFF          0
#define TCOON           1
#define TCIOFF          2
#define TCION           3

/* tcflush() and TCFLSH use these */
#define TCIFLUSH        0
#define TCOFLUSH        1
#define TCIOFLUSH       2

/* tcsetattr uses these */
#define TCSANOW         0
#define TCSADRAIN       1
#define TCSAFLUSH       2


#define _IOT_termios /* Hurd ioctl type field.  */ \
  _IOT (_IOTS (cflag_t), 4, _IOTS (cc_t), NCCS, _IOTS (speed_t), 2)


/**
 * Manipulate the underlying device parameters of special files.
 * fd is the file that you wish to manipulate, request is a 
 * device specific request code. The third parameter is an
 * untyped pointer to memory. Usually returns 0 on success. On
 * rare occasions it will return a non negative integer on
 * success. Returns -1 on failure.
 */
int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif
