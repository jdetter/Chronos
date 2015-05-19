#include "types.h"
#include "x86.h"
#include "stdlib.h"
#include "li_proxy.h"

int main(int argc, char** argv)
{
	li_printf("stdlib: strlen(\"Hello\"): %d\n", strlen("Hello"));
	li_printf("linux : strlen(\"Hello\"): %d\n", li_strlen("Hello"));
	li_printf("strncpy() for ABCEDF01234\n");
	char buff1[1000];
	char buff2[1000];
	memset(buff1, 0, 1000);
	memset(buff2, 0, 1000);
	
	strncpy(buff1, "ABCEDF\0 1234", 1000);
	li_strncpy(buff2, "ABCEDF\0 1234", 1000);
	li_printf("stdlib: %s\n", buff1);
	li_printf("linux : %s\n", buff2);

	memset(buff1, 0, 1000);
	memset(buff2, 0, 1000);
	
	li_strncpy(buff1, "Part one ", 1000);	
	li_strncpy(buff2, "Part one ", 1000);	

	strncat(buff1, "part 2.\0 This should not show up!!", 1000);
	li_strncat(buff2, "part 2.\0 This should not show up!!", 1000);

	li_printf("strncat\n");
	li_printf("stdlib: %s\n", buff1);
	li_printf("linux : %s\n", buff2);

	li_printf("strcmp\n");
	li_printf("stdlib: %d\n", strcmp("These \0 should be equal.", "These \0 should be equal."));
	li_printf("linux : %d\n", li_strcmp("These \0 should be equal.", "These \0 should be equal."));
	
	li_printf("atoi decimal\n");
	li_printf("stdlib: %d\n", atoi("123456789", 10));
	li_printf("linux : %d\n", li_strtol("123456789", (char**)0, 10));
	li_printf("stdlib: %d\n",      atoi("123456789abcdef", 16));
        li_printf("linux : %d\n", li_strtol("123456789abcdef", (char**)0, 16));
	li_printf("stdlib: %d\n",      atoi("123456789ABCDEF", 16));
        li_printf("linux : %d\n", li_strtol("123456789ABCDEF", (char**)0, 16));

	return 0;
}
