/**
 * boot-sign takes as an argument the file name of a 512 byte file that is
 * ready to be signed. This boot-sign should return a non zero value if
 * any error is encountered.

lseek.. file needs to be less than 510 bytes
put 55 AA at end
 */
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
	return 0;
}
