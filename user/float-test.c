#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	float f = 1000000.0f;
	int x;

	pid_t p = fork();

	for(x = 0;x < 10;x++)
	{
		f = f / 1.5f;
		printf("%d: number: %f\n", p, f);
	}

	return 0;
}
