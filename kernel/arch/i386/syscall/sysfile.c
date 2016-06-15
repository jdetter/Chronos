#include <sys/fcntl.h>
#include <sys/select.h>

#include "proc.h"
#include "syscall.h"
#include "ktime.h"
#include "drivers/rtc.h"

int sys_select_next_fd(int curr_fd, fd_set* set, int max_fd)
{
        for(;curr_fd < PROC_MAX_FDS && curr_fd < max_fd;curr_fd++)
        {
                if(FD_ISSET(curr_fd, set))
                        return curr_fd;
        }

        return -1;
}

int sys_select_fdset_move(fd_set* dst, fd_set* src, int max)
{
        FD_ZERO(dst);

        int set_count = 0;

        int fd;
        for(fd = 0;fd < max;fd++)
                if(FD_ISSET(fd, src))
                {
                        FD_SET(fd, dst);
                        set_count++;
                }

        return set_count;
}

int sys_select(void)
{
#ifdef DEBUG_SELECT
	cprintf("kernel: call to select.\n");
#endif
	int nfds;
	fd_set* readfds = NULL;
	fd_set* writefds = NULL;
	fd_set* exceptfds = NULL;
	struct timeval* timeout = NULL;

	fd_set ret_readfds;
	FD_ZERO(&ret_readfds);
	fd_set ret_writefds;
	FD_ZERO(&ret_writefds);
	fd_set ret_exceptfds;
	FD_ZERO(&ret_exceptfds);

	if(syscall_get_int(&nfds, 0))
		return -1;
	syscall_get_optional_ptr((void**)&readfds, 1);
	syscall_get_optional_ptr((void**)&writefds, 2);
	syscall_get_optional_ptr((void**)&exceptfds, 3);
	syscall_get_optional_ptr((void**)&timeout, 4);

#ifdef DEBUG_SELECT
	cprintf("nfds: %d  readfds? %d  writefds? %d exceptfds? %d timeout? %d\n", nfds, 
			readfds ? 1 : 0, 
			writefds ? 1 : 0, 
			exceptfds ? 1 : 0, 
			timeout ? 1 : 0);
	if(timeout)
		cprintf("\ttimeout: tv_sec: %d  tv_usec %d\n", timeout->tv_sec, timeout->tv_usec);
#endif

	int dev_found = 0;
	unsigned int endtime = 0;
	if(timeout)
		endtime = ktime_seconds() + timeout->tv_sec;
	while(!dev_found)
	{
		int fd;
		/* Check read fds if available */
		if(readfds)
		{
			fd = 0;
			while((fd = sys_select_next_fd(fd, readfds, nfds)) != -1)
			{
				/* Make sure this fd is okay */
				if(!fd_ok(fd)) 
				{
					fd++;
					continue;
				}

				/* Acquire the lock for this fd */
				slock_acquire(&rproc->fdtab[fd]->lock);

				switch(rproc->fdtab[fd]->type)
				{
					case FD_TYPE_FILE:
						/** 
						 * Files are always 
						 * ready for reading
						 */
						FD_SET(fd, &ret_readfds);
						dev_found = 1;
						break;
					case FD_TYPE_DEVICE: /** Going over the 80 limit here ---- */

						/* Does this device support read 'peek'? */
						if(rproc->fdtab[fd]->device->ready_read)
						{
							/* Does the device have something for us? */
							if(rproc->fdtab[fd]->device->ready_read(
										rproc->fdtab[fd]->device->context))
							{
								FD_SET(fd, &ret_readfds);
								dev_found = 1;
							}
						}

						break;
					case FD_TYPE_PATH:
					case FD_TYPE_NULL:
					default:
						break;
				}

				slock_release(&rproc->fdtab[fd]->lock);
				fd++;
			}
		}

		if(writefds)
		{
			fd = 0;
			while((fd = sys_select_next_fd(fd, writefds, nfds)) != -1)
			{
				/* Make sure this fd is okay */
                                if(!fd_ok(fd))
                                {
                                        fd++;
                                        continue;
                                }

                                /* Acquire the lock for this fd */
                                slock_acquire(&rproc->fdtab[fd]->lock);

				switch(rproc->fdtab[fd]->type)
				{
					case FD_TYPE_FILE:
						/** 
						 * Files are always 
						 * ready for writing
						 */
						dev_found = 1;
						FD_SET(fd, &ret_writefds);
						break;
					case FD_TYPE_DEVICE: /** Going over the 80 limit here ---- */

						/* Does this device support write 'peek'? */
						if(rproc->fdtab[fd]->device->ready_write)
						{
							/* Is this output device ready? */
							if(rproc->fdtab[fd]->device->ready_write(
										rproc->fdtab[fd]->device->context))
							{
								FD_SET(fd, &ret_writefds);
								dev_found = 1;
							}
						}

						break;
					case FD_TYPE_PATH:
					case FD_TYPE_NULL:
						break;
				}

				slock_release(&rproc->fdtab[fd]->lock);
				fd++;
			}
		}


		if(!dev_found)
		{
			/* yield for a cycle */
			yield();

			/* Is our timeout up? */
			if(timeout && ktime_seconds() >= endtime)
			{
#ifdef DEBUG_SELECT
				cprintf("select: timeout expired.\n");
#endif
				return 0;
			}
		}

	}	

	/* copy the return sets */
	int fd_count = 0;
	if(readfds) fd_count += sys_select_fdset_move(readfds, &ret_readfds, nfds);
	if(writefds) fd_count += sys_select_fdset_move(writefds, &ret_writefds, nfds);
	if(exceptfds) fd_count += sys_select_fdset_move(exceptfds, &ret_exceptfds, nfds);

#ifdef DEBUG_SELECT
	cprintf("select: value returned: %d\n", fd_count);
#endif

	return fd_count;
}

