#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <dirent.h>

#include "kstdlib.h"
#include "stdlock.h"
#include "file.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "syscall.h"
#include "chronos.h"
#include "proc.h"
#include "panic.h"

// #define DEBUG_SELECT
// #define DEBUG
// #define DEBUG_CONTENT

#ifdef RELEASE
# undef DEBUG
# undef DEBUG_SELECT
# undef DEBUG_CONTENT
#endif

int sys_link(void)
{
	const char* oldName;
	const char* newName;
	if(syscall_get_str_ptr(&oldName, 0)) return -1;
	if(syscall_get_str_ptr(&newName, 0)) return -1;

#ifdef DEBUG
	if(oldName && newName)
		cprintf("%s: created soft link %s -> %s\n",
				rproc->name, oldName, newName);
#endif

	return fs_link(oldName, newName);
}

int sys_stat(void)
{
	const char* path;
	struct stat* st;
	if(syscall_get_str_ptr(&path, 0)) return -1;
	if(syscall_get_buffer_ptr((void**) &st, sizeof(struct stat), 1)) return -1;
#ifdef DEBUG
	if(path) cprintf("%s: statting file: %s\n", rproc->name, path);
#endif
	inode i = fs_open(path, O_RDONLY, 0, 0, 0);
	if(i == NULL) 
	{
#ifdef DEBUG
		cprintf("%s: permission denied.\n", rproc->name);
#endif
		return -1;
	}
	fs_stat(i, st);
	fs_close(i);
	return 0;

}

/* int open(const char* path, int flags, ...); */
int sys_open(void)
{
	const char* path;
	int flags;
	mode_t mode = 0x0;

	if(syscall_get_str_ptr(&path, 0)) return -1; 
	if(syscall_get_int(&flags, 1)) return -1;
	int fd = fd_next(rproc);
	if(fd == -1) return -1;

#ifdef DEBUG
	cprintf("%s: opening file %s\n", rproc->name, path);
#endif

	/* Lock this file descriptor */
	slock_acquire(&rproc->fdtab[fd]->lock);

#ifdef O_PATH
	if(flags & O_PATH)
	{
		strncpy(rproc->fdtab[fd].path, path, FILE_MAX_PATH);
		rproc->fdtab[fd].type = FD_TYPE_PATH;
		slock_release(&rproc->fdtab[fd]->lock);
		return fd;
	}
#endif

	int created = 0;
	if(flags & O_CREAT)
	{
		if(syscall_get_int((int*)&mode, 2)) 
		{
			slock_release(&rproc->fdtab[fd]->lock);
			fd_free(rproc, fd);
			return -1;
		}

		created = fs_create(path, flags, mode, 
				rproc->uid, rproc->gid);
	}

	/* Check for O_EXCL */
	if((flags & O_CREAT) && (flags & O_EXCL) && created)
	{
		slock_release(&rproc->fdtab[fd]->lock);
		fd_free(rproc, fd);
		return -1;
	}

	rproc->fdtab[fd]->type = FD_TYPE_FILE;
	strncpy(rproc->fdtab[fd]->path, path, FILE_MAX_PATH);
	rproc->fdtab[fd]->flags = flags;
	rproc->fdtab[fd]->i = fs_open((char*) path, flags,
			mode, rproc->uid, rproc->uid);

	if(rproc->fdtab[fd]->i == NULL)
	{
		slock_release(&rproc->fdtab[fd]->lock);
		fd_free(rproc, fd);
		return -1;
	}

	struct stat st;
	fs_stat(rproc->fdtab[fd]->i, &st);

	if(S_ISDEV(st.st_mode))
	{
		/* Get the driver open */
		rproc->fdtab[fd]->type = FD_TYPE_DEVICE;
		struct devnode node;
		fs_read(rproc->fdtab[fd]->i, &node, 
				sizeof(struct devnode), 0);
		rproc->fdtab[fd]->device = dev_lookup(node.dev);
	}

	if(flags & O_DIRECTORY && !S_ISDIR(st.st_mode))
	{
		slock_release(&rproc->fdtab[fd]->lock);
		fd_free(rproc, fd);
		return -1;
	}

	if(flags & O_TRUNC && S_ISREG(st.st_mode))
		fs_truncate(rproc->fdtab[fd]->i, 0);

	rproc->fdtab[fd]->seek = 0;
#ifdef DEBUG
	cprintf("%s: opened file %s with fd %d\n", rproc->name, path, fd);
#endif

	slock_release(&rproc->fdtab[fd]->lock);

	return fd;
}

/* int close(int fd) */
int sys_close(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return -1;

#ifdef DEBUG
	cprintf("%s: closing file %d\n", rproc->name, fd);
#endif

	return close(fd);
}

int close(int fd)
{
	if(!fd_ok(fd)) return -1;

	slock_acquire(&rproc->fdtab[fd]->lock);
	if(rproc->fdtab[fd]->type == FD_TYPE_FILE)
	{
		fs_close(rproc->fdtab[fd]->i);
	}else if(rproc->fdtab[fd]->type == FD_TYPE_PIPE)
	{
		/* Do we need to free the pipe? */
		if(rproc->fdtab[fd]->pipe_type == FD_PIPE_MODE_WRITE)
			rproc->fdtab[fd]->pipe->write_ref--;
		if(rproc->fdtab[fd]->pipe_type == FD_PIPE_MODE_READ)
			rproc->fdtab[fd]->pipe->read_ref--;

		if(!rproc->fdtab[fd]->pipe->write_ref ||
				!rproc->fdtab[fd]->pipe->read_ref)
			rproc->fdtab[fd]->pipe->faulted = 1;
	}

	slock_release(&rproc->fdtab[fd]->lock);
	fd_free(rproc, fd);
	return 0;
}

/* int read(int fd, char* dst, size_t sz) */
int sys_read(void)
{
	int fd;
	char* dst;
	int sz;
	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int(&sz, 2)) return -1;
	if(syscall_get_buffer_ptr((void**)&dst, sz, 1)) return -1;

	/* Make sure this fd is valid */
	if(!fd_ok(fd)) return -1;

	/* Acquire the lock */
	slock_acquire(&rproc->fdtab[fd]->lock);

#ifdef DEBUG
	cprintf("%s:%d: doing read  for %d bytes.\n",
			rproc->name, rproc->pid, sz);
	cprintf("%s:%d: reading {%d} from file %s\n",
		rproc->name, rproc->pid,
		fd, rproc->fdtab[fd]->path);
#endif

	switch(rproc->fdtab[fd]->type)
	{
		default:
#ifdef DEBUG
			cprintf("%s: invalid fd %d\n", rproc->name, fd);
#endif 
			sz = -1;
			break;
		case FD_TYPE_FILE:
			if((sz = fs_read(rproc->fdtab[fd]->i, dst, sz,
							rproc->fdtab[fd]->seek)) < 0) 
			{
#ifdef DEBUG
				cprintf("READ FAILURE!\n");
#endif
				sz = -1;
				break;
			}

#ifdef DEBUG
			cprintf("%s: got %d bytes from read.\n", rproc->name, sz);
#endif
			break;
		case FD_TYPE_DEVICE:
			/* Check for read support */
			if(rproc->fdtab[fd]->device->read)
			{
				/* read is supported */
				sz = rproc->fdtab[fd]->device->read(dst,
						rproc->fdtab[fd]->seek, sz,
						rproc->fdtab[fd]->device->context);
			} else sz = -1;
			break;
		case FD_TYPE_PIPE:
			if(rproc->fdtab[fd]->pipe_type == FD_PIPE_MODE_READ)
				pipe_read(dst, sz, rproc->fdtab[fd]->pipe);
			else sz = -1;
			break;
	}

	if(sz > 0)
		rproc->fdtab[fd]->seek += sz;

#ifdef DEBUG_CONTENT
	cprintf("%s: CONTENTS |%s|\n", rproc->name, dst);
	cprintf("%s: new position in file: %d\n", 
			rproc->name, rproc->fdtab[fd]->seek);
#endif

	slock_release(&rproc->fdtab[fd]->lock);

	return sz;
}

/* int write(int fd, char* src, size_t sz) */
int sys_write(void)
{
	int fd;
	char* src;
	int sz;
	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int(&sz, 2)) return -1;
	if(syscall_get_buffer_ptr((void**)&src, sz, 1)) return -1;

	if(!fd_ok(fd)) return -1;
	slock_acquire(&rproc->fdtab[fd]->lock);

#ifdef DEBUG
	cprintf("%s:%d: doing write for %d bytes.\n",
			rproc->name, rproc->pid, sz);
	cprintf("%s:%d: Writing {%d} file: %s\n",
		rproc->name, rproc->pid,
		fd, rproc->fdtab[fd]->path);
#endif

	switch(rproc->fdtab[fd]->type)
	{
		default:
#ifdef DEBUG
			cprintf("%s: invalid fd %d\n", rproc->name, fd);
#endif 
			sz = -1;
			break;
		case FD_TYPE_FILE:
			if((sz = fs_write(rproc->fdtab[fd]->i, src, sz,
						rproc->fdtab[fd]->seek)) < 0)
			{
#ifdef DEBUG
				cprintf("%s: write to file %s failed!\n",
						rproc->name, 
						rproc->fdtab[fd]->path);
#endif
				sz = -1;
			}
			break;
		case FD_TYPE_PIPE:
			if(rproc->fdtab[fd]->pipe_type == FD_PIPE_MODE_WRITE)
				pipe_write(src, sz, rproc->fdtab[fd]->pipe);
			else sz = -1;
			break;
		case FD_TYPE_DEVICE:
			if(rproc->fdtab[fd]->device->write)
				sz = rproc->fdtab[fd]->device->write(src,
						rproc->fdtab[fd]->seek, sz, 
						rproc->fdtab[fd]->device->context);
			else sz = -1;
			break;
	}

	if(sz > 0)
		rproc->fdtab[fd]->seek += sz;

#ifdef DEBUG
	cprintf("%s:%d: New position in file: %d\n", 
			rproc->name, rproc->pid,
			rproc->fdtab[fd]->seek);
#endif
	slock_release(&rproc->fdtab[fd]->lock);
	return sz;
}

/* int lseek(int fd, int offset, int whence) */
int sys_lseek(void)
{
	int fd;
	off_t offset;
	int whence;
	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int((int*)&offset, 1)) return -1;
	if(syscall_get_int(&whence, 2)) return -1;
	if(!fd_ok(fd)) return -1;
	slock_acquire(&rproc->fdtab[fd]->lock);
#ifdef DEBUG
	cprintf("%s:%d: Seeking in file\n", rproc->name, rproc->pid);
	cprintf("%s:%d: File {%d}  %s\n", rproc->name, rproc->pid,
		fd, rproc->fdtab[fd]->path);
	cprintf("%s:%d: whence: %d  offset: %d\n",
		rproc->name, rproc->pid,
		whence, offset);
#endif

	int seek_pos;
	if(whence == SEEK_CUR){
#ifdef DEBUG
		cprintf("%s:%d: Setting seek to: %d\n", 
			rproc->name, rproc->pid, 
			offset + rproc->fdtab[fd]->seek);
#endif
		seek_pos = rproc->fdtab[fd]->seek + offset;
	}
	else if(whence == SEEK_SET){
#ifdef DEBUG
		cprintf("%s:%d: Setting seek to: %d\n", 
			rproc->name, rproc->pid, offset);
#endif

		seek_pos = offset;
	}
	else if(whence == SEEK_END){
		struct stat stat;
		fs_stat(rproc->fdtab[fd]->i, &stat);
		seek_pos = stat.st_size + offset;
#ifdef DEBUG
		cprintf("%s:%d: Setting seek from end to: %d\n", 
			rproc->name, rproc->pid, stat.st_size + offset);
#endif
	} else {
#ifdef DEBUG
		cprintf("%s:%d: ILLEGAL SEEK!! : %d\n",
			rproc->name, rproc->pid, whence);
#endif
		seek_pos = -1;
	}

	if(seek_pos >= 0)
		rproc->fdtab[fd]->seek = seek_pos;
	else {
#ifdef DEBUG
		cprintf("%s:%d: TRIED TO SET NEGATIVE SEEK!\n",
			rproc->name, rproc->pid);
#endif
	}

	slock_release(&rproc->fdtab[fd]->lock);
	return seek_pos;
}

/* int chmod(const char* path, mode_t perm) */
int sys_chmod(void)
{
	const char* path;
	mode_t mode;

#ifdef DEBUG
	cprintf("%s: chmodding file\n", rproc->name);
#endif

	if(syscall_get_str_ptr(&path, 0)) return -1;
	if(syscall_get_int((int*)&mode, 1)) return -1;

	return fs_chmod(path, mode);
}

/* int chown(const char* path, uid_t uid, gid_t gid) */
int sys_chown(void)
{
	const char* path;
	uid_t uid;
	gid_t gid;

	if(syscall_get_str_ptr(&path, 0)) return -1;
	if(syscall_get_int((int*)&uid, 1)) return -1;
	if(syscall_get_int((int*)&gid, 2)) return -1;

	return fs_chown(path, uid, gid);
}

/* int create(const char* file, mode_t permissions) */
int sys_create(void)
{
	const char* file;
	mode_t permissions;

	if(syscall_get_str_ptr(&file, 0)) return -1;
	if(syscall_get_int((int*)&permissions, 1)) return -1;
	return fs_create(file, 0, permissions, rproc->uid, rproc->uid);
}

/* int mkdir(const char* dir, mode_t permissions) */
int sys_mkdir(void)
{
	const char* dir;
	mode_t permissions;
	if(syscall_get_str_ptr(&dir, 0)) return -1;
	if(syscall_get_int((int*)&permissions, 1)) return -1;
	return fs_mkdir(dir, 0, permissions, rproc->uid, rproc->uid);
}

/* int unlink(const char* file) */
int sys_unlink(void)
{
	const char* file;
	if(syscall_get_str_ptr(&file, 0)) return -1;

#ifdef DEBUG
	if(file) 
		cprintf("%s: unlinking file %s\n", rproc->name,
				file);
#endif

	return fs_unlink(file);
}

/* int sys_rmdir(const char* dir) */
int sys_rmdir(void)
{
	const char* dir;
	if(syscall_get_str_ptr(&dir, 0)) return -1;
	return fs_rmdir(dir);
}

/* int mv(const char* orig, const char* dst) */
int sys_mv(void)
{
	/* Implement */
	panic("sys_mv not implemented!\n");
	return 0;
}

/* int fstat(int fd, struct file_stat* dst) */
int sys_fstat(void)
{
	int fd;
	struct stat* dst;

	if(syscall_get_buffer_ptr((void**)&dst, sizeof(struct stat), 1)) 
		return -1;
	if(syscall_get_int(&fd, 0)) return -1;
	if(!fd_ok(fd)) return -1;

#ifdef DEBUG
	cprintf("%s:%d: fstat on file fd: %d  path: %s\n", rproc->name, rproc->pid, 
		fd, rproc->fdtab[fd]->path);
#endif

	slock_acquire(&rproc->fdtab[fd]->lock);

	int close_on_exit = 0;
	inode i = NULL;
	/* Check file descriptor type */
	switch(rproc->fdtab[fd]->type)
	{
		case FD_TYPE_FILE:
			i = rproc->fdtab[fd]->i;
			break;
		case FD_TYPE_DEVICE:
		case FD_TYPE_PIPE:
#ifdef DEBUG
			cprintf("%s:%d: File is device.\n", rproc->name, rproc->pid);
#endif
			i = fs_open(rproc->fdtab[fd]->device->node, 
					O_RDONLY, 0x0, 0x0, 0x0);

			close_on_exit = 1;
			break;
		default:
#ifdef DEBUG
			cprintf("Fstat called on: %d\n", rproc->fdtab[fd]->type); 
#endif
			break;
	}

	int result = -1;
	if(i)
	{
		result = fs_stat(i, dst);
		if(close_on_exit)
			fs_close(i);
	}

	slock_release(&rproc->fdtab[fd]->lock);

	return result;
}

/* int readdir(int fd, struct old_linux_dirent* dirp, int count) */
int sys_readdir(void)
{
	/**
	 * A note on the return values:
	 *  +  1 = success
	 *  + -1 = failure
	 *  +  0 = failure (end of directory)
	 */

	int fd;
	struct old_linux_dirent* dirp;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_buffer_ptr((void**)&dirp, 
				sizeof(struct old_linux_dirent), 1))
		return -1;
	if(!fd_ok(fd)) return -1;
	if(rproc->fdtab[fd]->type != FD_TYPE_FILE)
		return -1;

	/* Acquire lock */
	slock_acquire(&rproc->fdtab[fd]->lock);

	struct dirent dir;
	int result = fs_readdir(rproc->fdtab[fd]->i, rproc->fdtab[fd]->seek, 
			&dir);
	if(result < 0) 
	{
		slock_release(&rproc->fdtab[fd]->lock);
		return -1;
	}

	/* Convert to old structure */
	dirp->d_ino = dir.d_ino;
	dirp->d_off = rproc->fdtab[fd]->seek;
	dirp->d_reclen = FILE_MAX_NAME;
	strncpy(dirp->d_name, dir.d_name, FILE_MAX_NAME);

	rproc->fdtab[fd]->seek++; /* Increment seek */
	slock_release(&rproc->fdtab[fd]->lock);
	return 1;
}

/* int getdents(int fd, struct chronos_dirent* dirp, int count) */
int sys_getdents(void)
{
	int fd;
	struct dirent* dirp;
	int count;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int((int*)&count, 2)) return -1;
	if(count < sizeof(struct dirent)) return -1;
	if(syscall_get_buffer_ptr((void**)&dirp, count, 1))
		return -1;
	if(!fd_ok(fd)) return -1;
	if(rproc->fdtab[fd]->type != FD_TYPE_FILE)
		return -1;

#ifdef DEBUG
	cprintf("%s:%d: getdents on fd {%d} file: %s\n", 
		rproc->name, rproc->pid, fd, rproc->fdtab[fd]->path);
#endif

	slock_acquire(&rproc->fdtab[fd]->lock);
	int result = fs_readdir(rproc->fdtab[fd]->i, 
		rproc->fdtab[fd]->seek, dirp);
	if(result < 0) 
	{
#ifdef DEBUG
		cprintf("%s:%d: ERROR READING DIRECTORY ENTRY\n",
				rproc->name, rproc->pid);
#endif
		slock_release(&rproc->fdtab[fd]->lock);
		return -1;
	}

	if(result > 0)
	{
		slock_release(&rproc->fdtab[fd]->lock);
#ifdef DEBUG
		cprintf("%s:%d: END OF DIRECTORY\n",
			rproc->name, rproc->pid);
#endif
		return 0;
	}

	/* Update the record length */
	dirp->d_reclen = sizeof(struct dirent);

	/* Convert to old structure */
	rproc->fdtab[fd]->seek++; /* Increment seek */
	slock_release(&rproc->fdtab[fd]->lock);
	return sizeof(struct dirent);
}

/* int pipe(int fd[2]) */
int sys_pipe(void)
{
	int* pipefd;
	if(syscall_get_buffer_ptr((void**)&pipefd, sizeof(int) * 2, 0))
		return -1;

	/* Try to get a pipe */
	pipe_t t = pipe_alloc();
	if(!t) return -1;

	/* Get a read fd */
	int read = fd_next(rproc);
	if(!fd_ok(read))
		return -1;
	int write = fd_next(rproc);
	if(!fd_ok(write))
		return -1;
	slock_acquire(&rproc->fdtab[read]->lock);
	slock_acquire(&rproc->fdtab[write]->lock);

	if(read >= 0)
	{
		rproc->fdtab[read]->type = FD_TYPE_PIPE;
		rproc->fdtab[read]->pipe_type = FD_PIPE_MODE_READ;
		rproc->fdtab[read]->pipe = t;
	}

	/* Get a write fd */
	if(write >= 0)
	{
		rproc->fdtab[write]->type = FD_TYPE_PIPE;
		rproc->fdtab[write]->pipe_type = FD_PIPE_MODE_WRITE;
		rproc->fdtab[write]->pipe = t;
	}

	t->write_ref = 1;
	t->read_ref = 1;
	pipefd[0] = read;
	pipefd[1] = write;
	
	slock_release(&rproc->fdtab[read]->lock);
	slock_release(&rproc->fdtab[write]->lock);
	
	return 0;
}

/* int dup(int fd) */
int sys_dup(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return -1;
	if(!fd_ok(fd)) return -1;
	int new_fd = fd_next(rproc);
	return dup2(new_fd, fd);
}

/* int dup2(int new_fd, int old_fd) */
int sys_dup2(void)
{
	int new_fd;
	int old_fd;
	if(syscall_get_int(&new_fd, 0)) return -1;
	if(syscall_get_int(&old_fd, 1)) return -1;
	if(!fd_ok(old_fd)) return 1; /* old_fd must be valid */
	return dup2(new_fd, old_fd);
}

int dup2(int new_fd, int old_fd)
{
	if(!fd_ok(old_fd))
		return -1;
	/* Make sure new_fd is closed */
	close(new_fd);
	/* Lock the old fd */
	slock_acquire(&rproc->fdtab[old_fd]->lock);
	/* Added a reference for this fd */
	rproc->fdtab[old_fd]->refs++;
	/* Create the mapping */
	rproc->fdtab[new_fd] = rproc->fdtab[old_fd];

	/* Modify references for other mechanisms */
	switch(rproc->fdtab[old_fd]->type)
	{
		default: break;
		case FD_TYPE_FILE:
			/* Increment inode references */
			rproc->fdtab[old_fd]->i->references++;
			break;
		case FD_TYPE_DEVICE:
			break;
		case FD_TYPE_PIPE:
			slock_acquire(&rproc->fdtab[old_fd]->pipe->guard);
			if(rproc->fdtab[old_fd]->pipe_type == FD_PIPE_MODE_WRITE)
				rproc->fdtab[old_fd]->pipe->write_ref++;
			if(rproc->fdtab[old_fd]->pipe_type == FD_PIPE_MODE_READ)
				rproc->fdtab[old_fd]->pipe->read_ref++;
			slock_release(&rproc->fdtab[old_fd]->pipe->guard);
			break;
	}

	/* Release the fd lock */
	slock_release(&rproc->fdtab[old_fd]->lock);
	return 0;
}

int sys_fchdir(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return -1;
	if(!fd_ok(fd)) return -1;
	slock_acquire(&rproc->fdtab[fd]->lock);
	switch(fd)
	{
		case FD_TYPE_FILE:
		case FD_TYPE_DEVICE:
		case FD_TYPE_PATH:
			break;
		default:
			slock_release(&rproc->fdtab[fd]->lock);
			return -1;
	}

	strncpy(rproc->cwd, rproc->fdtab[fd]->path, FILE_MAX_PATH);
	slock_release(&rproc->fdtab[fd]->lock);
	return 0;
}

int sys_fchmod(void)
{
	int fd;
	mode_t mode;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int((int*)&mode, 1)) return -1;
	if(!fd_ok(fd));
	slock_acquire(&rproc->fdtab[fd]->lock);

	switch(fd)
	{
		case FD_TYPE_FILE:
		case FD_TYPE_DEVICE:
		case FD_TYPE_PATH:
			break;
		default:
			slock_release(&rproc->fdtab[fd]->lock);
			return -1;
	}
			
	int result = fs_chmod(rproc->fdtab[fd]->path, mode);
	slock_release(&rproc->fdtab[fd]->lock);

	return result;
}

int sys_fchown(void)
{
	int fd;
	uid_t owner;
	gid_t group;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_short((short*)&owner, 1)) return -1;
	if(syscall_get_short((short*)&group, 2)) return -1;
	if(!fd_ok(fd)) return -1;
	slock_acquire(&rproc->fdtab[fd]->lock);

	switch(fd)
	{
		case FD_TYPE_FILE:
		case FD_TYPE_DEVICE:
		case FD_TYPE_PATH:
			break;
		default:
			slock_release(&rproc->fdtab[fd]->lock);
			return -1;
	}

	int result = fs_chown(rproc->fdtab[fd]->path, owner, group);
	slock_release(&rproc->fdtab[fd]->lock);
	return result;
}

int lchown(const char *pathname, uid_t owner, gid_t group)
{
	panic("Implementation needed for lchown.\n");
	return -1;
}

int sys_ioctl(void)
{
	int fd;
	unsigned long request;
	void* arg;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_long((long*)&request, 1)) return -1;
	if(sizeof(long) == sizeof(int))
	{
		if(syscall_get_int((int*)&arg, 2)) return -1;
	} else if(syscall_get_int((int*)&arg, 3)) return -1;

	if(!fd_ok(fd)) return -1;

	slock_acquire(&rproc->fdtab[fd]->lock);
	/* Is this a device? */
	if(rproc->fdtab[fd]->type != FD_TYPE_DEVICE)
	{
		slock_release(&rproc->fdtab[fd]->lock);
		return -1;
	}

	/* Does this device support ioctl? */
	if(!rproc->fdtab[fd]->device->ioctl)
	{
		slock_release(&rproc->fdtab[fd]->lock);
		return -1;
	}

	int result = rproc->fdtab[fd]->device->ioctl(request, 
			arg, rproc->fdtab[fd]->device->context);

	slock_release(&rproc->fdtab[fd]->lock);
	return result;
}

int sys_access(void)
{
	const char* pathname;
	mode_t mode;

	if(syscall_get_str_ptr(&pathname, 0)) return -1;
	if(syscall_get_int((int*)&mode, 1)) return -1;

#ifdef DEBUG
	cprintf("%s:%d: checking access to %s\n", rproc->name, rproc->pid,
		pathname);
#endif

	inode i = fs_open(pathname, O_RDONLY, 0x0, 
			rproc->ruid, rproc->rgid);

	if(!i) return -1; /* If the file dne, we can't open it! */

	struct stat st;
	fs_stat(i, &st);
	fs_close(i);

	return 0;
}

int sys_ttyname(void)
{
	int fd;
	char* buf;
	size_t sz;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int((int*)&sz, 2)) return -1;
	if(syscall_get_buffer_ptr((void**)&buf, sz, 1)) return -1;

	/* see if the fd is okay */
	if(!fd_ok(fd)) return -1;

	/* Lock this fd */
	slock_acquire(&rproc->fdtab[fd]->lock);
	/* Check to make sure the file descriptor is actually a device */
	if(rproc->fdtab[fd]->type != FD_TYPE_DEVICE
		|| !tty_check(rproc->fdtab[fd]->device)) 
	{
		slock_release(&rproc->fdtab[fd]->lock);
		return -1;
	}

	/* Move the mount point into buf */
	if(sz > FILE_MAX_PATH)
		sz = FILE_MAX_PATH;
	strncpy(buf, rproc->fdtab[fd]->device->node, sz);

	slock_release(&rproc->fdtab[fd]->lock);
	return 0;
}

int sys_fpathconf(void)
{
	int fd;
	int name;

	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int(&name, 1)) return -1;
	if(!fd_ok(fd)) return -1;

	struct file_descriptor* filed = rproc->fdtab[fd];
	const char* path = filed->path;

	return fs_pathconf(name, path);

}

int sys_pathconf(void)
{
	char* path;
	int name;

	if(syscall_get_str_ptr((const char**)&path, 0)) return -1;
	if(syscall_get_int(&name, 1)) return -1;

	return fs_pathconf(name, path);
}

int sys_lstat(void)
{
	const char* path;
	struct stat* st;
	if(syscall_get_str_ptr(&path, 0)) 
		return -1;
	if(syscall_get_buffer_ptr((void**) &st, sizeof(struct stat), 1)) 
		return -1;

#ifdef DEBUG
	cprintf("%s:%d: lstatting file %s\n", rproc->name, rproc->pid,
		path);
#endif

	inode i = fs_open(path, O_RDONLY, 0, 0, 0);
	if(i == NULL) 
	{
#ifdef DEBUG
		cprintf("%s:%d: Permission Denied.\n", 
			rproc->name, rproc->pid);
#endif
		return -1;
	}

	fs_stat(i, st);
	fs_close(i);
	return 0;

}

int sys_utime(void)
{
	cprintf("kernel: utime system call not implemented.\n");
	return -1;
}

int sys_utimes(void)
{
	cprintf("kernel: utimes system call not implemented.\n");
	return -1;
}

/* int fcntl(int fd, int action, ...) */
int sys_fcntl(void)
{
	int fd;
	int action;
	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int(&action, 1)) return -1;

	int i_arg;
	if(syscall_get_int(&i_arg, 2)) return -1;
	if(!fd_ok(fd)) return -1;

#ifdef DEBUG
	cprintf("%s: fcntl on fd %d, action %d, iarg: %d\n", 
			rproc->name, fd, action, i_arg);
#endif
	slock_acquire(&rproc->fdtab[fd]->lock);

	int result = 0;
	switch(action)
	{
		case F_DUPFD:
			i_arg = fd_next_at(rproc, i_arg);
			if(i_arg <= 0)
			{
				result = -1;
				break;
			}
			result = dup2(i_arg, fd);		
			break;
		case F_DUPFD_CLOEXEC:
			i_arg = fd_next_at(rproc, i_arg);
			if(i_arg <= 0)
			{
				result = -1;
				break;
			}
			result = dup2(i_arg, fd);
			/* Also set the close on exec flag */
			rproc->fdtab[i_arg]->flags |= O_CLOEXEC;
			break;
		case F_GETFD:
			result = rproc->fdtab[fd]->flags;
			break;
		case F_SETFD:
			rproc->fdtab[fd]->flags = i_arg;
			break;
		case F_GETFL:
			result = rproc->fdtab[fd]->flags;
			break;
		case F_SETFL:
			rproc->fdtab[fd]->flags = i_arg;
			break;
		default:
			cprintf("UNIMPLEMENTED FCNTL: %d\n", action);
			result = -1;
			break;
	}

	slock_release(&rproc->fdtab[fd]->lock);
	return result;
}

int sys_sysconf(void)
{
	int name;
	if(syscall_get_int(&name, 0)) return -1;

	long result = -1;
	switch(name)
	{
		case _SC_PAGESIZE:
			return PGSIZE;
		case _SC_PHYS_PAGES:
			return (int)(1 << 22);
		default:
#ifdef DEBUG
			cprintf("kernel: no such limit: %d\n", name);
#endif
			break;
	}

	return result;
}

int sys_ftruncate(void)
{
	panic("kernel: ftruncate not implemented yet.\n");
	return -1;
}

int sys_sync(void)
{
	fs_sync();
	return 0;
}


