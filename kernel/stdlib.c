/**
 * Authors: 
 *  + Mike Chargo <mike.chargo@sbcglobal.net> 
 *  + Andrea Myles <andrea-myles@uiowa.edu> 
 *  + John Detter <john@detter.com>
 * 
 * Basic set of standard library functions.
 */

#include <stdarg.h>
#include <sys/types.h>


int strlen(const char* str)
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
		if(str[x]>='Z' && str[x]<='A') 
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

char* strncpy(char* dst, const char* src, size_t sz)
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
	return dst; /* returns number of bytes */
}

int strncat(char* str1, char* str2, size_t sz)
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


int strncmp(const char* str1, const char* str2, size_t sz)
{
	int pos;
	for(pos = 0;str1[pos] && str2[pos] && pos < sz;pos++)
        {
                char str1_c = str1[pos];
                char str2_c = str2[pos];

                if(str1_c >= 'A' && str1_c <= 'Z')
                        str1_c += 32; /* convert to lower case */
                if(str2_c >= 'A' && str2_c <= 'Z')
                        str2_c += 32; /* convert to lower case */

                /* if they are equal, check next letter */
                if(str1_c == str2_c) continue;
                if(str1_c > str2_c) return 1; /* str1 comes after str2 */
                else return -1; /* str1 comes before str2 */
        }

	/* If pos == sz then they are equal */
	if(sz == pos )
		return 0;

	if(str1[pos])
		return 1;
	else return -1;
}

void memmove(void* dst, const void* src, size_t sz)
{
	/* Check for do nothing */
	if(dst == src) return;

	int x;
	char* cdst = dst;
	const char* csrc = src;
	/* Check for  <--s-<-->--d--> overlap*/
	if(src + sz > dst && src + sz < dst + sz)
	{
		for(x = sz - 1;x >= 0;x--)
			cdst[x] = csrc[x];
	}
	/* Otherwise we're fine */
	for(x = 0;x < sz;x++) cdst[x] = csrc[x];
}

void memset(void* dst, char val, size_t sz)
{
	char* udst = dst; /* creating udst variable to hold value for dst */
	int x;
        for(x = 0;x < sz;x++) /* for x is less than max memory size */
		udst[x] = val;	/* assigning each element of udst val */
}

int memcmp(void* buff1, void* buff2, size_t sz)
{
	char* ubuff1 = (char*)buff1; /* able to hold value other than void */
	char* ubuff2 = (char*)buff2; /* able to hold value other than void */
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

int atoi(const char* str)
{
	int negative = 1;
	if(*str == '-')
		negative = -1;

	int num = 0;

	const char* ptr = str;
	while(*ptr && *ptr >= '0' && *ptr <= '9')
	{
		int digit = *ptr - '0';
		num *= 10;
		num += digit;
		ptr++;
	}
	
	return negative * num;

}

float atof(char* str)
{
	return 0;
}

void kitoa(int val_signed, char* dst_c, size_t sz, int radix)
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
	if(val_signed < 0 && radix != 16)
	{
		neg = 1;
		val_signed = -val_signed;
	}

	if(radix == 16) neg = 0;
	int val = val_signed;

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
		dst_c[0] = '-';
		x = 1;	
	}
	int pos = strlen(dst) - 1;
	for(;x < sz && pos >= 0;x++, pos--)
		dst_c[x] = dst[pos];
	dst_c[sz - 1] = 0;
}

int vsnprintf(char* dst, size_t sz, const char* fmt, va_list list)
{
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
				int val = va_arg(list, int);
				kitoa(val, num_buff, 64, 10);
				int num_pos;
				for(num_pos = 0;num_buff[num_pos];num_pos++)
					dst[dst_index + num_pos] =
						num_buff[num_pos];
				dst_index += num_pos - 1;
			} else if(fmt[fmt_index + 1] == 'x'
					|| fmt[fmt_index + 1] == 'p')
			{
				char num_buff[64];
				int val = va_arg(list, int);
				kitoa(val, num_buff, 64, 16);
				int num_pos;
				for(num_pos = 0;num_buff[num_pos];num_pos++)
					dst[dst_index + num_pos] =
						num_buff[num_pos];
				dst_index += num_pos - 1;
			} else if(fmt[fmt_index + 1] == 'c')
			{
				char c = (char)va_arg(list, int);
				dst[dst_index] = c;
			} else if(fmt[fmt_index + 1] == 's')
			{
				char* s = va_arg(list, char*);
				int s_len = strncat(dst, s, sz);
				dst_index = s_len - 1;
			} else {
				dst[dst_index] = fmt[fmt_index];
				if(dst_index + 1 == sz) break;
				dst[dst_index + 1] = fmt[fmt_index + 1];
				dst_index++;
			}

			fmt_index++;
		} else dst[dst_index] = fmt[fmt_index];
	}

	dst[sz - 1] = 0;
	return dst_index;
}

int snprintf(char* dst, size_t sz, char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	int result =  vsnprintf(dst, sz, fmt, list);
	va_end(list);

	return result;
}

int trim(char* str)
{
	int x;
	for(x = 0;x < strlen(str);x++)
		if(str[x] != ' ')break;
	strncpy(str, str + x, strlen(str) + 1);
	for(x = strlen(str) - 1;x >= 0;x--)
		if(str[x] != ' ') break;
	str[x + 1] = 0;
	return strlen(str);
}

int ascii_char(char c)
{
	if(c < ' ') return 0;
	if(c == 127) return 0;
	if(c == 255) return 0;
	return 1;

}

char bcdtobin(char val)
{
	char high = (val & 0xF0) >> 4;
	char low = val & 0x0F;
	return (high * 10) + low;
}

int __log2(int value)
{
	int value_orig = value;
	/* Shift to the right until we hit a 1 */
	int x = 0;
	while(value != 1)
	{
		value >>= 1;
		x++;
	}

	/* Check work */
	if(1 << x != value_orig) return -1;

	return x;
}


/* Returns non zero if c is in delim, zero otherwise. */
static int strtok_r_delimiter(char c, const char* delim)
{
	while(delim && *delim)
	{
		if(*delim == c) 
			return 1;
		delim++;
	}

	return 0;
}

/* Trims any delim characters from str */
static char* strtok_r_initial_trim(char* str, const char* delim)
{
	/* Erase all prefixed delimiters */
	while(*str && strtok_r_delimiter(*str, delim))
	{
		*str = 0;
		str++;
	}	

	if(!strlen(str))
		return str;

	/* Erase all trailing delimiters */
	char* end = str + strlen(str) - 1;
	while(end > str && strtok_r_delimiter(*end, delim))
	{
		*end = 0;
		end--;
	}

	return str;
}

char* strtok_r(char* str, const char* delim, char** state)
{
	/* Make sure we are in a valid state */
	if(!state) return NULL;
	if(!str && !state) return NULL;
	if(!str && !*state) return NULL;

	/* We are done when state is pointing to a null character */
	char* search = NULL;
	if(!str)
	{
		/* state must be pointing to something valid */
		if(!*state) return NULL;

		/* We saved our search criteria from a previous run */
		search = *state;
	} else {
		/* Trim delimiters from start and end */
		str = strtok_r_initial_trim(str, delim);

		/* Was the string all delimiters? */
		if(!*str) return NULL;

		search = str;
	}

	/* Make sure our search criteria is non zero */
	if(!search)
		return NULL;

	/* Make sure our search critera isn't the end of the string */
	if(!*search)
		return NULL;

	/* Save the start of this string */
	char* start = search;

	/* Find the next delimiter */
	while(*search && !strtok_r_delimiter(*search, delim)) search++;

	/* Zero all delimiters until we hit a null or non delimiter */
	while(*search && strtok_r_delimiter(*search, delim))
	{
		*search = 0;
		search++;
	}

	/* Set the state */
	*state = search;

	return start;
}
