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

/* Memory allocator code */

#define M_AMT 0x5000 /* 5K heap space*/
#define M_MAGIC (void*)(0x43524E53)
int mem_init = 0;
/* A node in the free list. */
typedef struct free_node
{
	int sz; /* The size available here (not including this header) */
	struct free_node* next; /* The next free header in the list.*/
} free_node;

/* An allocated node. */
typedef struct alloc_node
{
	int sz; /* The size of this allocated region. */
	void* magic; /* Make sure this node isn't currupted. */
} alloc_node;

struct free_node* head; /* The head of the free list */
struct free_node* curr; /* Pointer to the current location. */

void* malloc(uint sz)
{
	if(!mem_init) minit();

	return NULL;
}

int mfree(void* ptr)
{
	if(!mem_init) minit();

	return 0;
}

void minit(void)
{
	mem_init = 1;
}
