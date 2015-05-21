typedef unsigned int uint;
#define NULL (void*)0

#include "li_proxy.h"
#include "stdlib.h"
#include "stdmem.h"

#define printf li_printf
#define MAX_ALLOC 4096
#define ALLOC_STRUCT 12

void test(char* test_name, int (*function)(void));
int basic_alloc(void);
int aligned_alloc(void);
int max_alloc(void);
int max_alloc2(void);

int free_null(void);
int free_alloc(void);

int main(int argc, char** argv)
{
	test("Basic Alloc  ", basic_alloc);
	test("Aligned Alloc", aligned_alloc);
	test("Max Alloc    ", max_alloc);
	test("Max Alloc 2  ", max_alloc2);

	return 0;
}

void test(char* test_name, int (*function)(void))
{
	printf("%s...\t\t", test_name);
	int result = function();
	if(result == 0) printf("[ OK ]\n");
	else printf("[FAIL]\n");
}

int basic_alloc(void)
{
	int* ptr = malloc(sizeof(int));
	if(ptr == NULL) return 1;
	return 0;
}

int aligned_alloc(void)
{
	int x;
	int* i_ptr;
	for(x = 0;x < 13;x++)
		i_ptr = malloc(sizeof(int));
	uint value = (uint)i_ptr;
	if((value & ~((uint)4095)) != value)
		return 1;
	if(i_ptr == NULL) return 1;
	return 0;
}

int max_alloc(void)
{
	int max = MAX_ALLOC - ALLOC_STRUCT;
	void* ptr = malloc(max);
	if(ptr == NULL) return 1;
	return 0;
}

int max_alloc2(void)
{
	int allocations = MAX_ALLOC / (ALLOC_STRUCT + sizeof(int));
	void* ptr;
	int x;
	for(x = 0;x < allocations;x++)
	{
		ptr = malloc(sizeof(int));
		if(ptr == NULL) return 1;
	}

	return 0;
}

int free_null(void)
{
	if(mfree(NULL) != 0) return 1;
	return 0;
}

int free_alloc(void)
{
	int x;
	void* ptr;
	for(x = 0;x < 1000000;x++)
	{
		ptr = malloc(4096);
		if(ptr == NULL) return 1;
		if(mfree(ptr) != 0) return 1;
	}

	return 0;
}
