#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

char* path = "/file.txt";

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

	char buffer[sizeof(int) * 128];

	/* Should end up with 64MB file */
	int iterations = 0x10000000;
	int x;
	for(x = 0;;)
	{
		int y;
		for(y = 0;y < 128;y++)
		{
			int val = x & ~(128 - 1);
			buffer[val] = x;
			x++;
		}

		if(write(fd, buffer, sizeof(int) * 128) != sizeof(int) * 128)
		{
			printf("Write failure!\n");
			return -1;
		}

		if(x >= iterations) break;
	}

	lseek(fd, 0, SEEK_SET);
	/* Check work */
        for(x = 0;;)
        {
		if(read(fd, buffer, sizeof(int) * 128) != sizeof(int) * 128)
                {
                        printf("Read failure!\n");
                        return -1;
                }

                int y;
                for(y = 0;y < 128;y++)
                {
                        int val = x & ~(128 - 1);
                        if(buffer[val] != x)
			{
				printf("Was expecing: %d, got %d\n", 
					x, buffer[val]);
				return -1;
			}
                        x++;
                }

                if(x >= iterations) break;
        }	

	return 0;
}

void test(int (*function)(void), char* message)
{
	printf("%s", message);
	if(function()) printf("[FAIL]\n");
	else printf("[ OK ]\n");
	
}

int main(int argc, char** argv)
{
	test(create_file,  "Creating file...\t\t\t\t\t\t\t");
	test(remove_file,  "Deleting file...\t\t\t\t\t\t\t");
	test(massive_file, "Massive file...\t\t\t\t\t\t\t");

	return 0;
}
