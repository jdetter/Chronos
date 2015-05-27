#include "types.h"
#include "x86.h"
#include "console.h"
#define dataReg 0x60
#define cntrlReg 0x64
int lShift = 0x12;
int rShift = 0x59;
#define enableKbd 0xf4
int set1 = 0xf1;
int shift = 0;
int cntrl = 0;
int alt = 0;
int caps = 0;
int cntrlkey = 0x14;
int altkey = 0x11;
int capskey = 0x58;
char sctoa(int scancode);
int kbd_init(){
	outb(cntrlReg, enableKbd);	
	outb(cntrlReg, set1);
	return 0;
}

char kbd_getc(){
	int status = (inb(cntrlReg) & 0x1);
	if(status == 0){
		return 0;
	}
	int scancode = inb(dataReg);
	int released = 0; 
	if(scancode == 0){
                return -1;
        }
	if(scancode & 0x80){
		released = 1;
		scancode &= 127;
		cprintf("Key released:\n");
	} else cprintf("Key pressed:\n");
	
	if(scancode == lShift || scancode == rShift){// LShift or Rshift pressed and not released
		if(released){
			shift = 0;
		}else{
			shift = 1;
		}
		/* get next scan code and convert to ascii then convert to uppercase*/
		
	} 
	else if(scancode == cntrlkey){// LShift or Rshift pressed and not released
		if(released){
			cntrl = 0;
		}else{
			cntrl = 1;
		}
		/* get next scan code and convert to ascii then convert to uppercase*/
		
	} 
	else if(scancode == altkey){// LShift or Rshift pressed and not released
		if(released){
			alt = 0;
		}else{
			alt = 1;
		}
		/* get next scan code and convert to ascii then convert to uppercase*/
		
	} 
	else if(scancode == capskey){// LShift or Rshift pressed and not released
		if(released){
			caps = 0;
		}else{
			caps = 1;
		}
		/* get next scan code and convert to ascii then convert to uppercase*/
		
	} else {
		if(released || cntrl || alt ){
			return 0;
		}
		else{
			char c = sctoa(scancode);
			if((shift && !caps)){
				if(c<=122 && c>=97){
					c -= 32; // uppercase
				}
				if(c==0){
					c -= 17;
				}
				if(c==1){
					c -= 16;
				}
				if(c==2){
					c+=14;
				}
				if(c == 3 || c == 4 || c == 5){
					c -=16;
				}
				if(c == 6){
					c += 40;
				}
				if(c == 7 || c == 9){
					c-= 17;
				}
				if( c == 8){
					c-=14;
				}
				if(c == '-'){
					c+=50;
				}
				if(c == '='){
					c+=18;
				}
				if(c >= 91 && c <=93){
					c += 32; 
				}
				if( c == '`'){
					c+=82;
				}
				if( c == ';'){
					c -=1;
				}
				if( c == '\''){
					c-=5;
				}
				if(c == ',' || c == '.'){
					c+=16;
				}
				if(c == '/'){
					c += 16;
				}

			}
			return c;
		}

		
	}
	return 0;
}

/*scan code to ascii */
char sctoa(int scancode){
	char kbdus[128] =
	{
	    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
	  '9', '0', '-', '=', '\b',	/* Backspace */
	  '\t',			/* Tab */
	  'q', 'w', 'e', 'r',	/* 19 */
	  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
	    0,			/* 29   - Control */
	  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
	 '\'', '`',   0,		/* Left shift */
	 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
	  'm', ',', '.', '/',   0,				/* Right shift */
	  '*',
	    0,	/* Alt */
	  ' ',	/* Space bar */
	    0,	/* Caps lock */
	    0,	/* 59 - F1 key ... > */
	    0,   0,   0,   0,   0,   0,   0,   0,
	    0,	/* < ... F10 */
	    0,	/* 69 - Num lock*/
	    0,	/* Scroll Lock */
	    0,	/* Home key */
	    0,	/* Up Arrow */
	    0,	/* Page Up */
	  '-',
	    0,	/* Left Arrow */
	    0,
	    0,	/* Right Arrow */
	  '+',
	    0,	/* 79 - End key*/
	    0,	/* Down Arrow */
	    0,	/* Page Down */
	    0,	/* Insert Key */
	    0,	/* Delete Key */
	    0,   0,   0,
	    0,	/* F11 Key */
	    0,	/* F12 Key */
	    0,	/* All other keys are undefined */
	};
	char letter= kbdus[scancode];
	return letter;
}


			
