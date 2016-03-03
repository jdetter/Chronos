#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	char* result = getlogin();
	printf("Address returned: 0x%x\n", (int)result);
	printf("Address of getlogin: 0x%x\n", (int)getlogin);
	printf("User: %s\n", result);
	char buff[512];
	printf("User: %s\n", buff);

	return 0;
}
