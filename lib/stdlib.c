#include "types.h"
#include "x86.h"
#include "stdarg.h"
#include "stdlib.h"

uint strlen(const char* str)
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

uint strncpy(char* dst, const char* src, uint sz)
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

int strcmp(const char* str1, const char* str2)
{
	while(*str1 && *str2) /* while str1 and str2 not equal to null */
	{
		char str1_c = *str1;
		char str2_c = *str2;

		if(str1_c >= 'A' && str1_c <= 'Z') 
			str1_c += 32; /* convert to lower case */
		if(str2_c >= 'A' && str2_c <= 'Z') 
			str2_c += 32; /* convert to lower case */

		str1++;
		str2++;

		/* if they are equal, check next letter */
		if(str1_c == str2_c) continue;
		if(str1_c > str2_c) return 1; /* str1 comes after str2 */
		else return -1; /* str1 comes before str2 */
	}

	if(!(*str1) && !(*str2)) return 0;
	else if(*str1) return 1;
	else return -1;
}


void memmove(void* dst, void* src, uint sz)
{
	/* Check for do nothing */
	if(dst == src) return;

	uint x;
	uchar* cdst = dst;
	uchar* csrc = src;
	/* Check for  <--s-<-->--d--> overlap*/
	if(src + sz > dst && src + sz < dst + sz)
	{
		for(x = sz - 1;x >= 0;x--)
			cdst[x] = csrc[x];
	}
	/* Otherwise we're fine */
	for(x = 0;x < sz;x++) cdst[x] = csrc[x];
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
	//int num = 0;
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

void itoa(int val_signed, char* dst_c, uint sz, uint radix)
{
	char dst[128];
	memset(dst, 0, 128);
	memset(dst_c, 0, sz);
	if(sz <= 1) return;

	if(val_signed == 0)
	{
		dst_c[0] = '0';
		dst_c[1] = 0;
		return;
	}

	int neg = 0;
	if(val_signed < 0)
	{
		neg = 1;
	}

	if(radix == 16) neg = 0;
	uint val = val_signed;

	int x;
	for(x = 0;x < 128 && val > 0;x++)
	{
		int mod = val % radix;
		char character = mod + '0';
		if(mod >= 10)
			character = (mod - 10) + 'A';
		dst[x] = character;
		val /= radix;
	}

	/* Flip the number into the buffer. */
	x = 0;
	if(neg)
	{
		dst_c[0] = 0;
		x = 1;	
	}
	int pos = strlen(dst) - 1;
	for(;x < sz && pos >= 0;x++, pos--)
		dst_c[x] = dst[pos];
	dst_c[sz - 1] = 0;
}

int va_snprintf(char* dst, uint sz, va_list* list, char* fmt)
{
	int arg = 1;
	
        int fmt_index = 0;
        int dst_index = 0;
        for(;dst_index < sz;fmt_index++, dst_index++)
        {
		if(fmt[fmt_index] == 0)
		{
			dst[dst_index] = 0;
			break;
		}
                if(fmt[fmt_index] == '%')
                {
                        if(fmt[fmt_index + 1] == '%')
                        {
                                dst[dst_index] = '%';
                        } else if(fmt[fmt_index + 1] == 'd')
                        {
                                char num_buff[64];
                                int val = (int)va_arg(list, arg);
                                itoa(val, num_buff, 64, 10);
                                int num_pos;
                                for(num_pos = 0;num_buff[num_pos];num_pos++)
                                        dst[dst_index + num_pos] =
                                                num_buff[num_pos];
                                dst_index += num_pos - 1;
                        } else if(fmt[fmt_index + 1] == 'x'
				|| fmt[fmt_index + 1] == 'p')
                        {
                                char num_buff[64];
                                int val = (int)va_arg(list, arg);
                                itoa(val, num_buff, 64, 16);
                                int num_pos;
                                for(num_pos = 0;num_buff[num_pos];num_pos++)
                                        dst[dst_index + num_pos] =
                                                num_buff[num_pos];
                                dst_index += num_pos - 1;
                        } else if(fmt[fmt_index + 1] == 'c')
			{
				char c = (int)va_arg(list, arg);
				dst[dst_index] = c;
			} else if(fmt[fmt_index + 1] == 's')
			{
				char* s = (char*)va_arg(list, arg);
                                int s_len = strncat(dst, s, sz);
				dst_index = s_len - 1;
			} else {
				dst[dst_index] = fmt[fmt_index];
				if(dst_index + 1 == sz) break;
				dst[dst_index + 1] = fmt[fmt_index + 1];
				dst_index++;
                        }

			arg++;
			fmt_index++;
                } else dst[dst_index] = fmt[fmt_index];
        }

        dst[sz - 1] = 0;
        return dst_index;
}

int snprintf(char* dst, uint sz, char* fmt, ...)
{
	va_list list;
	va_start(&list, (void**)&fmt);
        int result =  va_snprintf(dst, sz, &list, fmt);
	va_end(&list);

	return result;
}
