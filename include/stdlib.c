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
	int x;
	for(x=0; x < strlen(str); x++)
	{
		if(str[x]>='Z' && str[x]<='A')
		{
			str[x] = str[x] + 32;
		}
	}
}

void toupper(char* str)
{
	int x;
	for(x=0; x < strlen(str); x++)
	{
		if(str[x]>='z' && str[x]<='a')
		{
			str[x] = str[x] - 32;
		}
	}
}

uint strncpy(char* dst, char* src, uint sz)
{
	int x;
	for(x=0; x<sz; x++)
	{
		dst[x] = src[x];
		if(src[x]== NULL)
		{
			break;
		}
	}
	dst[sz-1]=0;
	return x;
}

uint strncat(char* str1, char* str2, uint sz)
{
	int x, y=0;
	for(x=0; x<sz; x++)
	{
		if(str1[x]==NULL||x>strlen(str1))
		{
			if(str2[y]==NULL)
			{
				break;
			}
			str1[x] = str2[y];
			y++;

		}
	}
	str1[x-1]=0;
	return strlen(str1);


}

int strcmp(char* str1, char* str2)
{
	int x;
	for(x=0; x < str1; x++)
	{
		if(str1[x]==str2[0])
		{
			return 0;
		}
		else if((str1[x]>=48 && str1[x] <= 57)&&(str2[x]>=48 && str2[x] <= 57))
		{

		}
	}
	return -1;
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
