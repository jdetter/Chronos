#include "stdlib.h"
#define GREY_COLOR 0x07
#define VID_ROWS 20
#define VID_COLS 80
#define VID_COLOR_BASE (int*) 0xB8000
int console_pos

int cprintf(char* fmt, ...)
{
	return 0;
}

void PrintCharacter(uint row, uint col, char character){
	int* vid_addr = VID_COLOR_BASE + (row * VID_COLS *2) +(col *2);
	*vid_addr = character;
	*(vid_addr +1) = GREY_COLOR;
}

/**
*WARNING: dst must be large enough to hold val
*/ 
void itoa(int val, char* dst, uint sz, unint base){
	memset(dst, 0, sz);
	
	if(val==0){
		dst[0] = '0';
		return;
	} 
	for(int i = 0; i<sz; i++){
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
	for(int i = 0 ; i<=strlen(dst)/2; i++){
		temp = dst[i];
		dst[i] = dst[strlen(dst)-1-i];
		dist[strlen(dst)-1-i] = temp;
		
	}
	
}

int cinit(){
	console_pos = 0;
	int row;
	int col;
	for(row=0; row< VID_ROWS; row++){
		for(col = 0; row<VID_COL; col++){
			printCharacter(row, col, 0x20);
		}
	}
}