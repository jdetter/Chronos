/**
 * Author: Amber Arnold <alarnold2@wisc.edu>
 *
 * Keyboard driver.
 *
 */

#include <stddef.h>

#include "x86.h"
#include "panic.h"
#include "drivers/pic.h"
#include "k/drivers/keyboard.h"

/** Uncomment this line for debugging */
// #define DEBUG

/* Values for the common special keys (setup in init) */
static int key_lctrl;
static int key_rctrl;
static int key_lalt;
static int key_ralt;
static int key_lshift;
static int key_rshift;
static int key_caps;

/* Keyboard special character state */
static int shift_enabled;
static int ctrl_enabled;
static int alt_enabled;
static int caps_enabled;
static int num_lock_enabled;

/* Keyboard commands */
#define KBD_ENABLE_SCANNING 0xF4
#define KBD_DISABLE_SCANNING 0xF4
#define KBD_GET_SET 0xF0

/* Responses */
#define KBD_ACK 0xFA
#define KBD_RESEND 0xFE

/* Statuses */
#define KBD_OUTPUT_FULL 0x01
#define KBD_INPUT_FULL 0x02

/* Ports */
#define KBD_DATA 0x60
#define KBD_CNTL 0x64
#define KBD_STAT 0x64

/* Scan sets */
char scan_set_1_alpha_numo[512] =
{
// 0    1   2    3    4    5    6    7    8    9    a    b    c    d    e   f
   0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=','\b','\t',//0
 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\r',  0, 'a', 's',//1
 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`',   0, '\\', 'z','x', 'c', 'v',//2
 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0,//3
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,//4
   0,   0,   0, '.',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//5
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//6
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//7
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//8
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//9
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//a
   0,   0,   0 ,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//b
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//c
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//d
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//e
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 //f
};

char scan_set_1_alpha_numo_shift[512] =
{
// 0    1   2    3    4    5    6    7    8    9    a    b    c    d    e   f
   0,   0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+','\b',   0,//0
 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}','\r',   0, 'A', 'S',//1
 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0, '|', 'Z', 'X', 'C', 'V',//2
 'B', 'N', 'M', '<', '>', '?',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0,//3
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,//4
   0,   0,   0, '.',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//5
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//6
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//7
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//8
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//9
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//a
   0,   0,   0 ,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//b
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//c
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//d
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,//e
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 //f
};

char scan_set_1_special_keys[512] =
{
	       0,       SKEY_ESC,	       0,	       0,//0X
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//1X
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,     SKEY_LCTRL,	       0,	       0,
	       0,	       0,	       0,	       0,//2X
	       0,	       0,	       0,	       0,
	       0,	       0,    SKEY_LSHIFT,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//3X
	       0,	       0,    SKEY_RSHIFT,	       0,
       SKEY_LALT,	       0,      SKEY_CAPS,        SKEY_F1,
         SKEY_F2,        SKEY_F3, 	 SKEY_F4,        SKEY_F5,
	 SKEY_F6,        SKEY_F7,        SKEY_F8,        SKEY_F9,//4X
	SKEY_F10,	       0,	       0,      SKEY_HOME,
     SKEY_UARROW,      SKEY_PGUP,	       0,    SKEY_LARROW,
	       0,    SKEY_RARROW,	       0,       SKEY_END,
     SKEY_DARROW,    SKEY_PGDOWN,	       0,    SKEY_DELETE,//5X
	       0,	       0,	       0,	SKEY_F11,
	SKEY_F12,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//6X
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//7X
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//8X
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//9X
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//AX
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//BX
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//CX
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//DX
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//EX
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,//FX
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0,
	       0,	       0,	       0,	       0

};

/* Special keys for set 1 */
#define SET1_KEY_CTRL   0x1D
#define SET1_KEY_ALT    0x38
#define SET1_KEY_CAPS   0x3A
#define SET1_KEY_LSHIFT 0x2A
#define SET1_KEY_RSHIFT 0x36

/* The chosen sets */
char* scan_set_alpha;
char* scan_set_alpha_shift;
char* scan_set_special;

/*** The key event handler
 *
 * pressed: if pressed is 1, the key was pressed. If pressed is 0, 
 *		the key was released.
 * special: whether val should be interprited as a special key
 * 		or as just a character value.
 * val: the value of the key. A list of macros can be found in keyboard.h
 * ctrl: whether or not control was enabled at the time of the event.
 * alt: whether or not alt was enabled at the time of the event.
 * shift: whether or not shift was enabled at the time of the event.
 * caps: whether or not caps lock was enabled at the time of the event.
 * ascii: The ascii code for the value 
 */
int (*shandle)(int pressed, int special, int val, int ctrl, int alt, 
		int shift, int caps, char ascii);

#define CHAR_FROM_SET(code) scan_set_alpha[code]
#define CHAR_FROM_SET_SHIFT(code) scan_set_alpha_shift[code]
#define CHAR_SPECIAL(code) scan_set_special[code]

#define IS_LOWERCASE(c) (c >= 'a' && c <= 'z')
#define IS_UPPERCASE(c) (c >= 'A' && c <= 'Z')
#define IS_ALPHA(c) (IS_LOWERCASE(c) || IS_UPPERCASE(c))
#define IS_NUMBER(c) (c >= '0' && c <= '9')

#define TO_LOWERCASE(c)  ((IS_UPPERCASE(c)) ? (c - 'a' + 'A') : (c))
#define TO_UPPERCASE(c)  ((IS_LOWERCASE(c)) ? (c - 'A' + 'a') : (c))

int kbd_init(void)
{

	pic_enable(INT_PIC_KEYBOARD);
	shift_enabled = 0;
	ctrl_enabled = 0;
	alt_enabled = 0;
	caps_enabled = 0;
	num_lock_enabled = 0;
	shandle = NULL; /* Handler starts as null. */

	/* Which set are we using? */
	int set = 1;

	if(set == 1)
	{
		key_lctrl = SET1_KEY_CTRL;
		key_rctrl = 0; /* disabled */
		key_lalt = SET1_KEY_ALT;
		key_ralt = 0; /* disabled */
		key_lshift = SET1_KEY_LSHIFT;
		key_rshift = SET1_KEY_RSHIFT;
		key_caps = SET1_KEY_CAPS;

		/* Select the proper sets */
		scan_set_alpha = scan_set_1_alpha_numo;
		scan_set_alpha_shift = scan_set_1_alpha_numo_shift;
		scan_set_special = scan_set_1_special_keys;
		
	} else return -1; /* No support */

	return 0;
}

char kbd_getc()
{
	/* loop until we get a key or no key is available */
	int status;
get_key:
	status = inb(KBD_STAT) & 0x1;

	/* Has an event occurred? */
	if(status == 0)
		return 0;

	/* retreive the scan code */	
	int scancode = inb(KBD_DATA);

	/* Did we actually get anything? */
	if(scancode == 0)
		goto get_key;

	/* Was the key released? */
	int released = 0; 
	if(scancode & 0x80)
	{
		released = 1;
		scancode &= 127;
	}

#ifdef DEBUG
	cprintf("kbd: received: 0x%x  pressed? %d\n", scancode, !released);
#endif

	/* Before any processing, check if it is a special key */
	char special_val = (unsigned char)CHAR_SPECIAL(scancode);
#ifdef DEBUG
	if(special_val)cprintf("kbd: Value was special!\n");
#endif
	if((alt_enabled || ctrl_enabled || special_val) && shandle)
	{
		int uns = (unsigned char) special_val;
		char ascii = CHAR_FROM_SET(scancode);
		shandle(!released, 1, uns, 
			ctrl_enabled, alt_enabled, 
			shift_enabled, caps_enabled, ascii);
		special_val = 1; /* Consume this event */
	}

	/*** Modify active modifiers */

	/* Check for shift key event */	
	if(scancode == key_lshift || scancode == key_rshift)
	{
		if(released)
			shift_enabled = 0;
		else
			shift_enabled = 1;
#ifdef DEBUG
		cprintf("kbd: shift modified: %d\n", shift_enabled);
#endif
		goto get_key;
	}

	/* Check for ctrl key event */
	if(scancode == key_lctrl || scancode == key_rctrl)
	{
		if(released)
			ctrl_enabled = 0;
		else
			ctrl_enabled = 1;
#ifdef DEBUG
		cprintf("kbd: ctrl modified: %d\n", ctrl_enabled);
#endif
		goto get_key;
	}

	/* Check for alt key event */
	if(scancode == key_lalt || scancode == key_ralt)
	{
		if(released)
			alt_enabled = 0;
		else
			alt_enabled = 1;
#ifdef DEBUG
		cprintf("kbd: alt modified: %d\n", alt_enabled);
#endif
		goto get_key;
	}

	/* Check for caps event */
	if(scancode == key_caps)
	{
		if(released)
			caps_enabled = 0;
		else
			caps_enabled = 1;
#ifdef DEBUG
		cprintf("kbd: caps modified: %d\n", caps_enabled);
#endif
		goto get_key;
	}

	/* If the key was released or it was special, lets skip it. */
	if(released || special_val) goto get_key;

	/* Which key was actually pressed? */
	char result;

	if(!shift_enabled && !caps_enabled)
	{
		/* Just get the normal key */
		result = CHAR_FROM_SET(scancode);
	} else if(!shift_enabled && caps_enabled)
	{
		/* We make letters only uppercase */
		result = CHAR_FROM_SET(scancode);
		if(IS_ALPHA(result) && IS_LOWERCASE(result))
			result = TO_UPPERCASE(result);
	} else if(shift_enabled && !caps_enabled)
	{
		/* Use the shift map */
		result = CHAR_FROM_SET_SHIFT(scancode);
	} else {
		/* All letters are lower case but swap syms */
		result = CHAR_FROM_SET_SHIFT(scancode);
		if(IS_ALPHA(result) && IS_UPPERCASE(result))
			result = TO_LOWERCASE(result);
	}

	return result;
}

void kbd_event_handler(int (*handle) (int pressed, int special, 
			int val, int ctrl, int alt, int shift, int caps,
			char ascii))
{
	/* assign the handler */
	shandle = handle;
}
