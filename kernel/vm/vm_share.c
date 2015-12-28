#if (!defined _ARCH_SHARE_PROVIDED_) && (defined _ALLOW_VM_SHARE_)

#include "vm.h"
#include "vm_share.h"

struct vm_share
{
	pypage_t page; /* Physical address of the shared page */
	int refs; /* How many people point to this page? */
	struct vm_share* next; /* Next vm_share in the list */
};

typedef struct vm_share vm_share_t;

int vm_pgshare(pypage_t pypage)
{
	return 0;
}

int vm_isshared(pypage_t pypage)
{
	return 0;
}

int vm_pgunshare(pypage_t pypage)
{
	return 0;
}


#endif
