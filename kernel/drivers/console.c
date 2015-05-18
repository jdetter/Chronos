#include "types.h"
#include "x86.h"
#include "stdlib.h"

#define GREY_COLOR 0x07
#define VID_ROWS 20
#define VID_COLS 80
#define VID_COLOR_BASE (int*) 0xB8000
int console_pos;

void printCharacter(uint row, uint col, char character);

int cprintf(char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int x;
	for(x = 0;x < strlen(fmt);x++)
	{
		if(fmt[x] == '%')
		{

		} else printNextCharacter(fmt[x]);
	}

	return 0;
}

void printNextCharacter(char character)
{
	int col = console_pos % VID_COLS;
	int row = (console_pos - col) / VID_COLS;
	printCharacter(row, col, character);
	console_pos++;	
}

void printCharacter(uint row, uint col, char character){
	int* vid_addr = VID_COLOR_BASE + (row * VID_COLS *2) +(col *2);
	*vid_addr = character;
	*(vid_addr +1) = GREY_COLOR;
}

/**
*WARNING: dst must be large enough to hold val
*/ 
void itoa(int val, char* dst, uint sz, uint base){
	memset(dst, 0, sz);
	
	if(val==0){
		dst[0] = '0';
		return;
	}
	int i; 
	for(i = 0; i<sz; i++){
		int mod = val % base;
		int ascii = 0;
		if(mod>9){
			ascii = 'A' + mod - 10;
		}
		else{
			ascii = mod + '0';
		}
		val = val/base;
		if(val == 0) break;

	}
	int temp;
	for(i = 0 ; i<=strlen(dst)/2; i++){
		temp = dst[i];
		dst[i] = dst[strlen(dst)-1-i];
		dst[strlen(dst)-1-i] = temp;
		
	}
	
}

int cinit(){
	console_pos = 0;
	int row;
	int col;
	for(row=0; row< VID_ROWS; row++){
		for(col = 0; row<VID_COLS; col++){
			printCharacter(row, col, 0x20);
		}
	}
}
