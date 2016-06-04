#include <chronos.h>

void main(int argc, char** argv)
{
	chronos_syscall(SYS_reboot, CHRONOS_RB_SHUTDOWN);

	return 0;
}
