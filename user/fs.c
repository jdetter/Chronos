#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

char* path = "file.txt";

int create_file(void)
{
	int fd = open(path, O_CREAT | O_RDWR, 0644);
	if(fd < 0) return -1;
	close(fd);
	
	return 0;
}

int remove_file(void)
{
	return unlink(path);
}

int massive_file(void)
{
	int fd = open(path, O_CREAT | O_RDWR, 0644);
	if(fd < 0)
		return -1;

	int buffer[128];
	int iterations = 0x10000;

	int x;
	for(x = 0;x < iterations;x++)
	{
		int y;
                for(y = 0;y < 128;y++)
			buffer[y] = x;

		if(write(fd, buffer, 128 * sizeof(int)) != 128 * sizeof(int))
		{
			printf("Write failure! %d\n", x);
			close(fd);
			return -1;
		}
	}

	lseek(fd, 0, SEEK_SET);
	for(x = 0;x < iterations;x++)
	{
		if(read(fd, buffer, 128 * sizeof(int)) != 128 * sizeof(int))
		{
			printf("Read failure! %d\n", x);
			close(fd);
			return -1;
		}

		int y;
		for(y = 0;y < 128;y++)
		{
			if(x != buffer[y])
			{
				printf("Mismatch failure! %d\n", x);
				close(fd);
				return -1;
			}
		}
	}

	close(fd);
	return 0;
}

int mkdir_test(void)
{
	unlink(path);
	if(mkdir(path, 0777))
	{
		printf("Couldn't create directory\n");
		return -1;
	}
	if(unlink(path))
	{
		printf("Couldn't remove directory!\n");
		return -1;
	}
	return 0;
}

void test(int (*function)(void), char* message)
{
	printf("%s", message);
	fflush(stdout);
	if(function()) printf("[FAIL]\n");
	else printf("[ OK ]\n");
	fflush(stdout);
	
}

int main(int argc, char** argv)
{
	test(create_file,  "Creating file...\t\t\t\t\t\t\t");
	test(remove_file,  "Deleting file...\t\t\t\t\t\t\t");
	test(mkdir_test,   "Make Directory...\t\t\t\t\t\t\t\t");
	test(massive_file, "Massive file...\t\t\t\t\t\t\t\t");

	return 0;
}
