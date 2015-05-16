#include "types.h"
#include "x86.h"
#include "stdlib.h"

#include <stdio.h>

void test(char*, int (*)(void));
int strlen_1(void);
int strlen_2(void);
int tolower_1(void);
int tolower_2(void);
int toupper_1(void);
int toupper_2(void);

int main(int argc, char** argv)
{
	test("strlen test 1", strlen_1);
	test("strlen null test", strlen_2);
	test("tolower alpha", tolower_1);
	test("tolower all char", tolower_2);
	test("toupper alpha", toupper_1);
	test("toupper all char", toupper_2);

	return 0;
}

void test(char* test_name, int (*function)(void))
{
	printf("%s...\t\t", test_name);
	int result = function();
	if(result == 0) printf("[ OK ]\n");
	else printf("[FAIL]\n");
}

int strlen_1(void)
{
	char* str = "Hello, world!";
	int length = 13;
	int test = strlen(str);
	if(length != test) return 1;

	return 0;
}

int strlen_2(void)
{
	char* str = "";
	if(strlen(str) != 0) return 1;
	return 0;
}

int tolower_1(void)
{
        char* str = "ABCDEFGHIJKLMNOP";
        tolower(str);
	int x;
	for(x = 0;x < 16;x++)
		if(str[x] < 'a') return 1;
        return 0;
}

int tolower_2(void)
{
        char* str = "ABCDE!@#$%.,;'[]{}adfsaj24352345";
        tolower(str);
        int x;
        for(x = 0;x < 32;x++)
                if(str[x] >= 'A' && str[x] <= 'Z') return 1;
        return 0;
}

int toupper_1(void)
{
        char* str = "abcdefghijklmnop";
        tolower(str);
        int x;
        for(x = 0;x < 16;x++)
                if(str[x] >= 'a') return 1;
        return 0;
}

int toupper_2(void)
{
        char* str = "ABCDE!@#$%.,;'[]{}adfsaj24352345";
        tolower(str);
        int x;
        for(x = 0;x < 32;x++)
                if(str[x] >= 'a' && str[x] <= 'z') return 1;
        return 0;
}

int strncpy_1(void)
{
	return 0;
}
