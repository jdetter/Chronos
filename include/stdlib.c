#include "types.h"
#include "x86.h"
#include "stdlib.h"

uint strlen(char* str)
{
	int x;
	for(x = 0;str[x] != 0;x++);
	return x;	
}

void tolower(char* str)
{

}

void toupper(char* str)
{

}

uint strncpy(char* dst, char* src, uint sz)
{
	return 0;
}

uint strncat(char* str1, char* str2, uint sz)
{
	return 0;
}

int strcmp(char* str1, char* str2)
{
	return 0;
}

void memmove(void* dst, void* src, uint sz)
{

}

void memset(void* dst, char val, uint sz)
{

}

int memcmp(void* buff1, void* buff2, uint sz)
{
	return 0;
}

int atoi(char* str, int radix)
{
	return 0;
}

float atof(char* str)
{
	return 0;
}

int snprintf(char* dst, uint sz, char* fmt, ...)
{
	return 0;
}
