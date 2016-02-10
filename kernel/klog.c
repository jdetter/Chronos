#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include "klog.h"
#include "stdlock.h"
#include "fsman.h"

static slock_t klog_lock;
static struct klog klog_table[NKLOG];

void klog_init(void)
{
	slock_init(&klog_lock);
	memset(klog_table, 0, sizeof(struct klog) * NKLOG);
	fs_mkdir("/var", 0, 0700, 0, 0);
        fs_mkdir("/var/log", 0, 0700, 0, 0);
}

klog_t klog_alloc(int append, char* path)
{
	char* base = "/var/log/";
	char new_path[256];
	strncpy(new_path, base, 256);
	strncat(new_path, path, 256);

	inode i = fs_open(new_path, O_RDWR | O_CREAT, 0600, 0, 0);
	off_t offset = 0;
	
	if(!i) return NULL;

	if(append)
	{
		struct stat st;
		fs_stat(i, &st);
		offset = st.st_size;
	}

	klog_t result = NULL;

	slock_acquire(&klog_lock);

	int x;
	for(x = 0;x < NKLOG;x++)
	{
		if(!klog_table[x].valid)
		{
			result = klog_table + x;
			result->valid = 1;
			result->offset = offset;
			result->i = i;
			break;
		}
	}

	slock_release(&klog_lock);

	return result;
}

void klog_free(klog_t log)
{
	if((char*)log >= (char*)klog_table &&
		(char*)log < (char*)(klog_table + NKLOG))
	{
		fs_close(log->i);
		log->valid = 0;
	}
}

int klog_write(klog_t log, void* buffer, size_t sz)
{
	int b = 0;
	while(b != sz)
	{
		b += fs_write(log->i, (char*)buffer + b, sz - b, 
			log->offset + b);
	}

	log->offset += b;

	return b;
}
