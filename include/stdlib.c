#include "types.h"
#include "x86.h"
#include "stdlib.h"
#include "stdarg.h"

uint strlen(char* str)
{
	int x;
	for(x = 0;str[x] != 0;x++); /* counts the length of the string */
	return x;	/* returns length of string */
}

void tolower(char* str)
{
	int x;
	for(x=0; x < strlen(str); x++) /* for the given length of the string */
	{
		if(str[x]>='Z' && str[x]<='A') /* if string is within the ASCII upper case alphabet, add 32 to convert to lower case equivalent */
		{
			str[x] = str[x] + 32;
		}
	}
}

void toupper(char* str)
{
	int x;
	for(x=0; x < strlen(str); x++) /* for the given length of the string */
	{
		if(str[x]>='z' && str[x]<='a') /* if string is within the ASCII lower case alphabet, subtract 32 to convert to upper case equivalent */
		{
			str[x] = str[x] - 32;
		}
	}
}

uint strncpy(char* dst, char* src, uint sz)
{
	int x;
	for(x=0; x<sz; x++) /* iterates through source until reaches max memory size */
	{
		dst[x] = src[x]; /* set destination to source */
		if(src[x] == 0) /* break if null */
		{
			break;
		}
	}
	dst[sz-1]=0; /* ensures last element is null char */
	return x; /* returns number of bytes */
}

uint strncat(char* str1, char* str2, uint sz)
{
	int start = strlen(str1); /* set variable as length of string */
	int x;
	for(x = 0;x < sz;x++) /* traverse through until max mem */
	{
		str1[start + x] = str2[x]; /* begin string 2 at end next element of string 1 */
		if(str2[x] == 0) break; /* break at null */
	}

	str1[sz - 1] = 0; /* ensures last element is null of new string */

	return x + start; /* returns length of combined string */
}

int strcmp(char* str1, char* str2)
{
	while(*str1 && *str2) /* while str1 and str2 not equal to null */
	{
		if(*str1 >= 'A' && *str1 <= 'Z') /* if str1 is a capital letter of the alphabet */
			*str1 += 32; /* convert to lower case */
		if(*str2 >= 'A' && *str2 <= 'Z') /* if str2 is a capital letter of the alphabet */
			*str2 += 32; /* convert to lower case */
		if(*str1 == *str2) continue; /* if they are equal, continue */
		if(*str1 > *str2) return 1; /* if str1 comes after str2, return 1 */
		else return -1; /* if str1 is before str2, return -1 */
	}

	return 0;
}

void memmove(void* dst, void* src, uint sz)
{
	uchar* usrc = (uchar*)src; /* able to hold value other than void */
	uchar* udst = (uchar*)dst; /* able to hold value other than void */

	uchar c_src[sz]; /* creating source array length sz */
	int x;
	for(x = 0;x < sz;x++) /* for x is less than max memory size */
		c_src[x] = usrc[x]; /* making a copy of source */
	for(x = 0;x < sz;x++) /* for x is less than max memory size */
		udst[x] = c_src[x]; /* assigning the udst the value of the copy made from usrc[x] */
}

void memset(void* dst, char val, uint sz)
{
	uchar* udst = dst; /* creating udst variable to hold value for dst */
	int x;
        for(x = 0;x < sz;x++) /* for x is less than max memory size */
		udst[x] = val;	/* assigning each element of udst val */
}

int memcmp(void* buff1, void* buff2, uint sz)
{
	uchar* ubuff1 = (uchar*)buff1; /* able to hold value other than void */
	uchar* ubuff2 = (uchar*)buff2; /* able to hold value other than void */
	int x;
	for(x=0; x<sz; x++) /* for x is less than max memory size */
	{
		if(ubuff1[x]!=ubuff2[x]) /* compares buff1 and buff2 at current index, if unequal returns -1 */
		{
			return -1;
		}
	}
	return 0; /* if buff1 equals buff2, return 0 */
}

int atoi(char* str, int radix)
{
	int x;
	int negative = 1;
	int total = 0;
	int num = 0;
	if(str[0]=='-')
	{
		negative = -1;
	}
	if(radix==16)
	{
		for(x=0; x<strlen(str); x++)
		{
			if(str[x] >= 'A' && str[x] <= 'Z')/* if str1 is a capital letter of the alphabet */
			{
				str[x] += 32; /* convert to lower case */
			}
			if((str[x]<'a' && str[x] > 'f')&&(str[x]> '9' || str[x] <'0'))
			{
				break;
			}
			if(str[x]=='a')
			{
				total *= radix;
				total = total + 10;
			}
			else if(str[x]=='b')
			{
				total *= radix;
				total = total + 11;
			}
			else if(str[x]=='c')
			{
				total *= radix;
				total = total + 12;
			}
			else if(str[x]=='d')
			{
				total *= radix;
				total = total + 13;
			}
			else if(str[x]=='e')
			{
				total *= radix;
				total = total + 14;
			}
			else if(str[x]=='f')
			{
				total *= radix;
				total = total + 15;
			}
			else if(str[x]<='9'&&str[x]>='0')
			{
				total *= radix;
				total = total + str[x]-'0';
			}
		}

	}
	else
	{
		for(x=0; x<strlen(str); x++)
		{

			if(radix == 2 && str[x]!= '0' && str[x]!= '1')
			{
				break;
			}
			else if(radix == 10 && (str[x]> '9' || str[x] <'0'))
			{
				break;
			}
			else
			{
				total *= radix;
				total = total + str[x]-'0';
			}

		}
	}
	return total * negative;
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
