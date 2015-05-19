#include "types.h"
#include "x86.h"
#include "stdlib.h"
#include "stdarg.h"

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
		if(src[x] == 0)
		{
			break;
		}
	}
	dst[sz-1]=0;
	return x;
}

uint strncat(char* str1, char* str2, uint sz)
{
	int start = strlen(str1);
	int x;
	for(x = 0;x < sz;x++)
	{
		str1[start + x] = str2[x];
		if(str2[x] == 0) break;
	}

	str1[sz - 1] = 0;

	return x + start;
}

int strcmp(char* str1, char* str2)
{
	while(*str1 && *str2)
	{
		if(*str1 >= 'A' && *str1 <= 'Z')
			*str1 += 32;
		if(*str2 >= 'A' && *str2 <= 'Z')
			*str2 += 32;
		if(*str1 == *str2) continue;
		if(*str1 > *str2) return 1;
		else return -1;
	}

	return 0;
}

void memmove(void* dst, void* src, uint sz)
{
	uchar* usrc = (uchar*)src;
	uchar* udst = (uchar*)dst;

	uchar c_src[sz];
	int x;
	for(x = 0;x < sz;x++)
		c_src[x] = usrc[x];
	for(x = 0;x < sz;x++)
		udst[x] = c_src[x];
}

void memset(void* dst, char val, uint sz)
{
	uchar* udst = dst;
	int x;
        for(x = 0;x < sz;x++)
		udst[x] = val;	
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

void itoa(int val, char* dst, uint sz, uint radix)
{
	memset(dst, 0, sz);
	if(sz <= 1) return;

	if(val == 0)
	{
		*dst = '0';
		return;
	}

	int x;
	for(x = 0;x < sz;x++)
	{
		if(x == 0 && val < 0)
		{
			dst[x] = '-';
			val = -val;
			continue;
		}

		int mod = val % radix;
		char character = mod + '0';
		if(mod > 10)
			character = mod - 10 + 'A';
		dst[x] = character;
	}
	dst[sz - 1] = 0;
}

int snprintf(char* dst, uint sz, char* fmt, ...)
{
	va_list list;
	va_start(&list, (void**)&fmt);
	int arg = 0;

	int fmt_index = 0;
	int dst_index = 0;
	for(;dst_index < sz;fmt_index++, dst_index++)
	{
		if(fmt[fmt_index] == '%')
		{
			if(fmt[fmt_index + 1] == '%')
			{
				dst[dst_index] = '%';
			} else if(fmt[fmt_index + 1] == 'd')
			{
				int val = *((int*)va_arg(list, arg));
				itoa(val, dst + dst_index, sz - dst_index, 10);
			}
			fmt_index++;
		} else dst[dst_index] = fmt[fmt_index];
	}

	dst[sz - 1] = 0;
	va_end(list);
	return dst_index;
}
