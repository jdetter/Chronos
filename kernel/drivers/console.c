#include "types.h"
#include "x86.h"
#include "stdlib.h"

#define MAX_NUM 15
#define GREY_COLOR 0x07
#define VID_ROWS 20
#define VID_COLS 80
#define VID_COLOR_BASE (int*) 0xB8000
int console_pos;


void printCharacter(uint row, uint col, char character);
/* do string char %%(to print % ) and pointer addresses*/
int cprintf(char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	//int arg_pos = 0; I don't think we need this
	int x;
	char *s;
	void *pointer;
	int pa;
	for(x = 0;x < strlen(fmt);x++)
	{
		if(fmt[x] == '%')
		{
			if(fmt[x + 1] == 'd')
			{
				char number[MAX_NUM];
				snprintf(number, MAX_NUM, "%d", va_arg(args, int));
				for(;number;number++)printNextCharacter(*number);
				//arg_pos++;
			} else if(fmt[x + 1] == 'x')
			{
				char number[MAX_NUM];
				snprintf(number, MAX_NUM, "%x", va_arg(args, int));
				for(;number;number++)printNextCharacter(*number);
				//arg_pos++;
			}else if(fmt[x + 1] == 'c')
			{
				printNextCharacter(va_arg(args, char));
				//arg_pos++;
			}else if(fmt[x + 1] == 's')
			{
				s = va_arg(args, char *);
				for(int i = 0; i< strlen(s); i++)
				{
					printNextCharacter(char[i]);
				}
				//arg_pos++;
			}else if(fmt[x + 1] == '%')
			{
				printNextCharacter('%');
				//arg_pos++;
			}else if(fmt[x + 1] == 'p')
			{
				char number[MAX_NUM];
				pointer = va_arg(args, void *);
				pa = (int) pointer;
				snprintf(number, MAX_NUM, "%x",  pa);
				for(;number;number++)printNextCharacter(*number);
				//arg_pos++;
			}
			
			
		} else if(fmt[x] == '\n'){
			
			console_pos = console_pos + (VID_COLS - (console_pos % VID_COLS));
		}
		else 
		{
			printNextCharacter(fmt[x]);
		}
	}
	va_end(args);
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
 * NOT USING THIS
 * 
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
*/

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
