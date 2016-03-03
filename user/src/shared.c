#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdint.h>

extern void* __mmap(void* hint, uint sz, int protection,
   int flags, int fd, off_t offset);

int main(int argc, char** argv)
{
	printf("Calling mmap...\n");
	void* page = __mmap(NULL, 0x1000, 7, 1, -1, 0);
	int* num = page;

	if(!page)
		printf("mmap failed!\n");

	printf("mmap success!\n");
	*num = 0;

	int f = fork();

	if(f == 0)
	{
		printf("In child.\n");
		(*num)++;
		int last = 1;
		while(last < 50)
		{
			while(*num == last);
			last = *num + 1;
			(*num)++;
			printf("Child at %d\n", last);
		}
		printf("Child done\n");
	} else if(f > 0)
	{
		printf("In parent.\n");
		int last = 0;;
		while(last < 50)
		{
			while(*num == last);
			last = *num + 1;
			(*num)++;
			printf("Parent at %d\n", last);
		}
		printf("Parent done!\n");

		/* Wait for child */
		waitpid(f, NULL, 0);
	} else {
		printf("Fork failed!\n");
	}

	return 0;
}
