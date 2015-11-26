#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	char buffer[512];

	while(1)
	{
		int bytes = read(1, buffer, 511);
		buffer[bytes] = 0;
		printf("tester got: %s", buffer);
	}
}
