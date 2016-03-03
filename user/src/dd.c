#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	char* in_file = NULL;
	char* out_file = NULL;

	int in_fd = 1; /* stdin */
	int out_fd = 2; /* stdout */
	int count = -1; /* until in is eof */
	int bs = 512; /* Write size */

	int x;
	for(x = 1;x < argc; x++)
	{
		if(!strncmp("if=", argv[x], 3))
		{
			in_file = argv[x] + 3;
		} else if(!strncmp("of=", argv[x], 3))
		{
			out_file = argv[x] + 3;
		} else if(!strncmp("bs=", argv[x], 3))
		{
			bs = atoi(argv[x] + 3);
		} else if(!strncmp("count=", argv[x], 6))
		{
			count = atoi(argv[x] + 6);
		}
	}

	if(in_file)
	{
		in_fd = open(in_file, O_RDONLY);

		if(in_fd < 0)
		{
			printf("dd: no such file: %s\n", in_file);
			return -1;
		}
	}

	if(out_file)
	{
		out_fd = open(out_file, O_WRONLY | O_CREAT, 0644);

		if(out_fd < 0)
		{
			printf("dd: permission denied: %s\n", out_file);
			return -1;
		}
	}

	printf("count: %d\n", count);
	printf("bs:    %d\n", bs);
	if(in_file) printf("in:   %s\n", in_file);
	if(out_file) printf("out:  %s\n", out_file);

	char buff[bs];

	int done = 0;
	int blocks_total = 0;
	int bytes_transfered = 0;
	while(blocks_total != count)
	{
		if(blocks_total != 0 && blocks_total % 10 == 0)
			printf("Progress:  %d/%d blocks copied\n", blocks_total, count);

		int rb = 0;
		do
		{
			int res = read(in_fd, buff + rb, bs - rb);
			if(res <= 0)
			{
				done = 1;
				break;
			}

			rb += res;
		} while(rb != bs);

		int wb = 0;
		do
		{
			int res = write(out_fd, buff + wb, rb - wb);

			if(res < 0)
			{
				fprintf(stderr, "dd: write failed!\n");
				done = 1;
				break;
			}

			wb += res;
		} while(wb < rb);

		bytes_transfered += rb;
		if(done) break;
		blocks_total++;
	}

	printf("0x%x | %d bytes read\n", bytes_transfered, bytes_transfered);
	printf("0x%x | %d bytes written\n", bytes_transfered, bytes_transfered);

	return 0;
}
