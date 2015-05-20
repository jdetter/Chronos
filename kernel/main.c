
#include "types.h"
#include "x86.h"
#include "pic.h"
#include "pit.h"

int main_stack();

/* Entry point for the kernel */
void main()
{
	/* WARNING: we don't have a proper stack right now. */

	/* Setup virtual memory and allocate a proper stack. */


	main_stack();
	/* main_stack doesn't return. */
}

int main_stack()
{
	/* We now have a proper kernel stack */
}
