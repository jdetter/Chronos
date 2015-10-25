#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/mman.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
	printf("Fork - WaitPid - Exit test...\t\t\t\t\t\t");
	int pid = fork();
	if(pid == 0)
	{
		exit(5);
	} else if(pid > 0)
	{
		int ret_val;
		waitpid(pid, &ret_val, 0x0);
		if(ret_val == 5)
			printf("[ OK ]\n");
		else printf("[FAIL]\n");
	} else {
		printf("[FAIL]\n");
	}

	printf("MMap test...\t\t\t\t\t\t\t\t");

	/* 1KB area */
	int x;
	char* area1 = (char*)mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	for(x = 0;x < 1024;x++)area1[x] = 1;
	/* 10MB area */
	char* area2 = (char*)mmap(NULL, 1024 * 1024 * 10, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	for(x = 0;x < 1024 * 1024 * 10;x++)area2[x] = 2;

	char mmap_failed = 0;
	/* Test areas */
	for(x = 0;x < 1024;x++) if(area1[x] != 1) mmap_failed = 1;
	for(x = 0;x < 1024 * 1024 * 10;x++) if(area2[x] != 2) mmap_failed = 1;

	if(!area1 || !area2 || area1 == area2 || mmap_failed)
		printf("[FAIL]\n");
	else printf("[ OK ]\n");

	printf("Fixed mapping test...\t\t\t\t\t\t\t");
	uint fixed = 0x300000;
	void* result = mmap((void*)fixed, 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0, 0);
	
	if((uint)result != fixed)
		printf("[FAIL]\n");
	else printf("[ OK ]\n");

	pid = fork();
	char* readonly_area = (char*)mmap(NULL, 1024, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if(pid == 0)
	{
		for(x = 0;x < 1024;x++)readonly_area[x] = 1;	
		exit(5);
	} else if(pid  > 0)
	{
		int result;
		waitpid(pid, &result, 0x0);
		printf("Read only pages...\t\t\t\t\t\t\t");
		if(result == 5)
		{
			printf("[FAIL]\n");
		} else {
			printf("[ OK ]\n");
		}
	} else {
		printf("FORK FAILED.\n");
		exit(1);
	}

	printf("Bad fixed mapping test...\t\t\t\t\t\t");
	void* bad_area = mmap((void*)9000, 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0, 0);	

	if(bad_area) printf("[FAIL]\n");
	else printf("[ OK ]\n");

	exit(0);
}
