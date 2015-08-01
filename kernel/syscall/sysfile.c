#include "types.h"
#include "stdlock.h"
#include "file.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "syscall.h"
#include "stdlib.h"
#include "chronos.h"
#include "proc.h"
#include "panic.h"

extern struct proc* rproc;

/**
 * Find an available file descriptor.
 */
static int find_fd(void)
{
	int x;
	for(x = 0;x < MAX_FILES;x++)
		if(rproc->file_descriptors[x].type == 0x0)
			return x;
	return -1;
}

/** Check to see if an fd is valid */
static int fd_ok(int fd)
{
	if(fd < 0 || fd >= MAX_FILES)
		return 1;
	if(rproc->file_descriptors[fd].type)
		return 0;
	return 1;
}

int sys_link(void)
{
	const char* oldName;
	const char* newName;
	if(syscall_get_str_ptr(&oldName, 0)) return -1;
	if(syscall_get_str_ptr(&newName, 0)) return -1;
	return fs_link(oldName, newName);

}

int sys_stat(void)
{
	const char* path;
	struct stat* st;
	if(syscall_get_str_ptr(&path, 0)) return -1;
	if(syscall_get_buffer_ptr((void**) &st, sizeof(struct stat), 1)) return -1;
	inode i = fs_open(path, O_RDONLY, 0, 0, 0);
	if(i == NULL) return -1;
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
	int fd = find_fd();
	if(fd == -1){
		return -1;
	}

	if(flags & O_PATH)
	{
		strncpy(rproc->file_descriptors[fd].path, path, FILE_MAX_PATH);
		rproc->file_descriptors[fd].type = FD_TYPE_PATH;
		return -1;
	}

	int created = 0;
	if(flags & O_CREATE)
	{
		if(syscall_get_int((int*)&mode, 2)) return -1;
		created = fs_create(path, flags, mode, rproc->uid, rproc->gid);
	}

	/* Check for O_EXCL */
	if(flags & O_CREATE && flags & O_EXCL && !created)
		return -1;

	rproc->file_descriptors[fd].type = FD_TYPE_FILE;
	rproc->file_descriptors[fd].flags = flags;
	rproc->file_descriptors[fd].i = fs_open((char*) path, flags,
			mode, rproc->uid, rproc->uid);
	if(rproc->file_descriptors[fd].i == NULL)
	{
		memset(rproc->file_descriptors + fd, 0, sizeof(struct file_descriptor));
		return -1;
	}

	struct stat st;
	fs_stat(rproc->file_descriptors[fd].i, &st);

	if(S_ISDEV(st.st_mode))
	{
		/* Get the driver open */
		rproc->file_descriptors[fd].type = FD_TYPE_DEVICE;
		struct devnode node;
		fs_read(rproc->file_descriptors[fd].i, &node, sizeof(struct devnode), 0);
		rproc->file_descriptors[fd].device = dev_lookup(node.dev);
	}

	if(flags & O_DIRECTORY && !S_ISDIR(st.st_mode))
	{
		memset(rproc->file_descriptors + fd, 0, sizeof(struct file_descriptor));
		return -1;
	}

	if(flags & O_TRUNC && S_ISREG(st.st_mode))
		fs_truncate(rproc->file_descriptors[fd].i, 0);

	rproc->file_descriptors[fd].seek = 0;
	return fd;
}

/* int close(int fd) */
int sys_close(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return -1;
	return close(fd);
}

int close(int fd)
{
	if(fd >= MAX_FILES || fd < 0){return -1;}
	if(rproc->file_descriptors[fd].type == FD_TYPE_FILE)
		fs_close(rproc->file_descriptors[fd].i);
	else if(rproc->file_descriptors[fd].type == FD_TYPE_PIPE)
	{
		/* Do we need to free the pipe? */
		if(rproc->file_descriptors[fd].pipe_type == FD_PIPE_MODE_WRITE)
			rproc->file_descriptors[fd].pipe->write_ref--;
		if(rproc->file_descriptors[fd].pipe_type == FD_PIPE_MODE_READ)
			rproc->file_descriptors[fd].pipe->read_ref--;

		if(!rproc->file_descriptors[fd].pipe->write_ref ||
				!rproc->file_descriptors[fd].pipe->read_ref)
			rproc->file_descriptors[fd].pipe->faulted = 1;
	}
	rproc->file_descriptors[fd].type = 0x00;
	return 0;
}

/* int read(int fd, char* dst, uint sz) */
int sys_read(void)
{
	int fd;
	char* dst;
	int sz;
	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int(&sz, 2)) return -1;
	if(syscall_get_buffer_ptr((void**)&dst, sz, 1)) return -1;

	switch(rproc->file_descriptors[fd].type)
	{
	case 0x00: return -1;
	case FD_TYPE_FILE:
		if((sz = fs_read(rproc->file_descriptors[fd].i, dst, sz,
				rproc->file_descriptors[fd].seek)) < 0) {
			return -1;
		}
		break;
	case FD_TYPE_DEVICE:
		/* Check for read support */
		if(rproc->file_descriptors[fd].device->io_driver.read)
		{
			/* read is supported */
			sz = rproc->file_descriptors[fd].device->
				io_driver.read(dst,
				rproc->file_descriptors[fd].seek, sz,
				rproc->file_descriptors[fd].device->
				io_driver.context);
		} else return -1;
		break;
	case FD_TYPE_PIPE:
		if(rproc->file_descriptors[fd].pipe_type == FD_PIPE_MODE_READ)
			pipe_read(dst, sz, rproc->file_descriptors[fd].pipe);
		else return -1;
		break;
	default: return -1;
	}

	rproc->file_descriptors[fd].seek += sz;
	return sz;
}

/* int write(int fd, char* src, uint sz) */
int sys_write(void)
{
	int fd;
	char* src;
	int sz;
	if(syscall_get_int(&fd, 0)) return -1;
	if(syscall_get_int(&sz, 2)) return -1;
	if(syscall_get_buffer_ptr((void**)&src, sz, 1)) return -1;

	switch(rproc->file_descriptors[fd].type)
	{
	default: return -1;
	case FD_TYPE_FILE:
		if(fs_write(rproc->file_descriptors[fd].i, src, sz,
				rproc->file_descriptors[fd].seek) == -1)
			return -1;
		break;
	case FD_TYPE_PIPE:
		if(rproc->file_descriptors[fd].pipe_type == FD_PIPE_MODE_WRITE)
			pipe_write(src, sz, rproc->file_descriptors[fd].pipe);
		else return -1;
	case FD_TYPE_DEVICE:
		if(rproc->file_descriptors[fd].device->io_driver.write)
			sz = rproc->file_descriptors[fd].device->
				io_driver.write(src,
				rproc->file_descriptors[fd].seek, sz, 
				rproc->file_descriptors[fd].device->
				io_driver.context);
		else return -1;
		break;
	}

	rproc->file_descriptors[fd].seek += sz;
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
	if(rproc->file_descriptors[fd].type != FD_TYPE_FILE){
		return -1;
	}
	int seek_pos;
	if(whence == SEEK_CUR){
		seek_pos = rproc->file_descriptors[fd].seek + offset;
	}
	else if(whence == SEEK_SET){
		seek_pos = offset;
	}
	else if(whence == SEEK_END){
		struct stat stat;
		fs_stat(rproc->file_descriptors[fd].i, &stat);
		seek_pos = stat.st_size + offset;
	} else {
		return -1;
	}
	if(seek_pos < 0){ seek_pos = 0;}
	rproc->file_descriptors[fd].seek = seek_pos;
	return seek_pos;
}


/* int chmod(const char* path, uint perm) */
int sys_chmod(void)
{
	const char* path;
	mode_t mode;

	if(syscall_get_str_ptr(&path, 0)) return -1;
	if(syscall_get_int((int*)&mode, 1)) return -1;
	return fs_chmod(path, mode);
}

/* int chown(const char* path, uint uid, uint gid) */
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

/* int create(const char* file, uint permissions) */
int sys_create(void)
{
	const char* file;
	uint permissions;

	if(syscall_get_str_ptr(&file, 0)) return -1;
	if(syscall_get_int((int*)&permissions, 1)) return -1;
	return fs_create(file, 0, permissions, rproc->uid, rproc->uid);
}

/* int mkdir(const char* dir, uint permissions) */
int sys_mkdir(void)
{
	const char* dir;
	uint permissions;
	if(syscall_get_str_ptr(&dir, 0)) return -1;
	if(syscall_get_int((int*)&permissions, 1)) return -1;
	fs_mkdir(dir, 0, permissions, rproc->uid, rproc->uid);
	return 0;
}

/* int unlink(const char* file) */
int sys_unlink(void)
{
	const char* file;
	if(syscall_get_str_ptr(&file, 0)) return -1;
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
	if(fd < 0 || fd >= MAX_FILES) return -1;

	int close_on_exit = 0;
	inode i = NULL;
	/* Check file descriptor type */
	switch(rproc->file_descriptors[fd].type)
	{
	case FD_TYPE_FILE:
		i = rproc->file_descriptors[fd].i;
		break;
	case FD_TYPE_DEVICE:
	case FD_TYPE_PIPE:
		i = fs_open(rproc->file_descriptors[fd].device->node, 
			O_RDONLY, 0x0, 0x0, 0x0);
		if(!i) return -1;
		close_on_exit = 1;
		break;
	default:
		cprintf("Fstat called on: %d\n", rproc->file_descriptors[fd].type); 
		return -1;
	}

	int result = fs_stat(i, dst);

	if(close_on_exit)fs_close(i);

	return result;
}

/* int readdir(int fd, struct old_linux_dirent* dirp, uint count) */
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
	if(fd_ok(fd)) return -1;
	if(rproc->file_descriptors[fd].type != FD_TYPE_FILE)
		return -1;

	struct dirent dir;
	int result = fs_readdir(rproc->file_descriptors[fd].i, 
		rproc->file_descriptors[fd].seek, &dir);
	if(result < 0) return -1;
	if(result == 1) return 0;

	/* Convert to old structure */
	dirp->d_ino = dir.d_ino;
	dirp->d_off = rproc->file_descriptors[fd].seek;
	dirp->d_reclen = FILE_MAX_NAME;
	strncpy(dirp->d_name, dir.name, FILE_MAX_NAME);

	rproc->file_descriptors[fd].seek++; /* Increment seek */
	return 1;
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
	int read = find_fd();
	if(read >= 0)
	{
		rproc->file_descriptors[read].type = FD_TYPE_PIPE;
		rproc->file_descriptors[read].pipe_type = FD_PIPE_MODE_READ;
		rproc->file_descriptors[read].pipe = t;
	}

	/* Get a write fd */
	int write = find_fd();
	if(write >= 0)
	{
		rproc->file_descriptors[write].type = FD_TYPE_PIPE;
		rproc->file_descriptors[write].pipe_type = FD_PIPE_MODE_WRITE;
		rproc->file_descriptors[write].pipe = t;
	}

	if(read < 0 || write < 0)
	{
		if(read >= 0)
			rproc->file_descriptors[read].type = 0x0;
		if(write >= 0)
			rproc->file_descriptors[write].type = 0x0;
		pipe_free(t);
		return -1;
	}
	t->write_ref = 1;
	t->read_ref = 1;
	pipefd[0] = read;
	pipefd[1] = write;

	return 0;
}

/* int dup(int fd) */
int sys_dup(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return -1;
	int new_fd = find_fd();
	return dup2(new_fd, fd);
}

/* int dup2(int new_fd, int old_fd) */
int sys_dup2(void)
{
	int new_fd;
	int old_fd;
	if(syscall_get_int(&new_fd, 0)) return -1;
	if(syscall_get_int(&old_fd, 1)) return -1;
	return dup2(new_fd, old_fd);
}

int dup2(int new_fd, int old_fd)
{
	if(new_fd < 0 || new_fd >= MAX_FILES) return -1;
	if(old_fd < 0 || old_fd >= MAX_FILES) return -1;
	/* Make sure new_fd is closed */
	close(new_fd);
	memmove(rproc->file_descriptors + new_fd,
			rproc->file_descriptors + old_fd,
			sizeof(struct file_descriptor));
	switch(rproc->file_descriptors[old_fd].type)
	{
	default: return -1;
	case FD_TYPE_FILE:
		rproc->file_descriptors[old_fd].i->references++;
		break;
	case FD_TYPE_DEVICE:break;
	case FD_TYPE_PIPE:
		slock_acquire(&rproc->
				file_descriptors[old_fd].pipe->guard);
		if(rproc->file_descriptors[old_fd].pipe_type ==
				FD_PIPE_MODE_WRITE)
			rproc->file_descriptors[old_fd].
			pipe->write_ref++;
		if(rproc->file_descriptors[old_fd].pipe_type
				== FD_PIPE_MODE_READ)
			rproc->file_descriptors[old_fd].
			pipe->read_ref++;
		slock_release(&rproc->
				file_descriptors[old_fd].pipe->guard);
		break;
	}

	return 0;
}

int fchdir(int fd)
{
	if(fd < 0) return -1;
	if(fd >= MAX_FILES) return -1;
	switch(fd)
	{
	case FD_TYPE_FILE:
	case FD_TYPE_DEVICE:
	case FD_TYPE_PATH:
		break;
	default:
		return -1;
	}
	strncpy(rproc->cwd, rproc->file_descriptors[fd].path, FILE_MAX_PATH);
	return 0;
}

int fchmod(int fd, mode_t mode)
{
	if(fd < 0) return -1;
	if(fd >= MAX_FILES) return -1;
	switch(fd)
	{
	case FD_TYPE_FILE:
	case FD_TYPE_DEVICE:
	case FD_TYPE_PATH:
		break;
	default:
		return -1;
	}
	return fs_chmod(rproc->file_descriptors[fd].path, mode);
}

int fchown(int fd, uid_t owner, gid_t group)
{
	if(fd < 0) return -1;
	if(fd >= MAX_FILES) return -1;
	switch(fd)
	{
	case FD_TYPE_FILE:
	case FD_TYPE_DEVICE:
	case FD_TYPE_PATH:
		break;
	default:
		return -1;
	}
	return fs_chown(rproc->file_descriptors[fd].path, owner, group);
}

int lchown(const char *pathname, uid_t owner, gid_t group)
{

	return -1;
}


