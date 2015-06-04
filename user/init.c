#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"

int main(int argc, char** argv)
{
	char* message = "Hello, world!\n";
	write(1, message, strlen(message));

	exit();
	return 0;
}
