#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
	DIR* d;
	d = opendir("/");

	struct dirent* ent;
	printf("Size of dirent: %lu\n", sizeof(struct dirent));
	while((ent = readdir(d)))
	{
		printf("%s\n", ent->d_name);
	}

	closedir(d);
	return -1;
}
