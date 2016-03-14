#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

/**
 * Initilize the keyboard controller
 */
int kbd_init(void);

/**
 * Get an ASCII character from the keyboard if possible. If an ASCII character
 * is not ready, 0 will be returned.
 */
char kbd_getc(void);

/**
 * Assign a key event handler to the keyboard driver.
 */
void kbd_event_handler(int (*handle)
		(int pressed, int special, int val, int ctrl, int alt, 
			int shift, int caps, char ascii));

/* Special keys for the above event handler */
#define SKEY_LARROW	0x01
#define SKEY_RARROW	0x02
#define SKEY_UARROW	0x03
#define SKEY_DARROW	0x04
#define SKEY_RCTRL	0x05
#define SKEY_LCTRL	0x06
#define SKEY_RSHIFT	0x07
#define SKEY_LSHIFT	0x08
#define SKEY_RALT	0x09
#define SKEY_LALT	0x0A
#define SKEY_CAPS	0x0B
#define SKEY_ESC	0x0C
#define SKEY_END	0x0D
#define SKEY_HOME	0x0E
#define SKEY_PGUP	0x0F
#define SKEY_PGDOWN	0x10
#define SKEY_DELETE	0x11
#define SKEY_SUPER	0x12

#define SKEY_F1		0x80
#define SKEY_F2		0x81
#define SKEY_F3		0x82
#define SKEY_F4		0x83
#define SKEY_F5		0x84
#define SKEY_F6		0x85
#define SKEY_F7		0x86
#define SKEY_F8		0x87
#define SKEY_F9		0x88
#define SKEY_F10	0x89
#define SKEY_F11	0x8A
#define SKEY_F12	0x8B


#endif
