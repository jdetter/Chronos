#ifdef ARCH_i386
#include "../../kernel/arch/i386/include/vm.h"
#else
#error "Invalid architecture."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define perror() perror("boot-imager")

static void err(char* msg)
{
	printf("boot-imager: %s\n", msg);
	exit(1);
}

static void usage(void)
{
	printf("USAGE: boot-imager -o output_file input_file [--64]\n\n");
	exit(0);
}

static void print_command(char** arglist)
{
	while(*arglist)
	{
		printf("%s ", *arglist);
		arglist++;
	}

	printf("\n");
}

static void create_section(const char* input_file, const char* section_name)
{
	char output_file[64];
	snprintf(output_file, 64, "%s%s", input_file, section_name);

	int pid = fork();

	if(pid > 0)
	{
		int status;
		if(waitpid(pid, &status, 0) < 0)
			err("waitpid failure.");

		if(WEXITSTATUS(status) != 0)
			err("child exec failure (objcopy).");

	} else if(pid == 0)
	{
		char* args[8];
		args[0] = "objcopy";
		args[1] = "-O";
		args[2] = "binary";
		args[3] = "-j";
		args[4] = (char*)section_name;
		args[5] = (char*)input_file;
		args[6] = (char*)output_file;
		args[7] = NULL;

		print_command(args);
		execvp(args[0], args);

		perror();
		exit(1);
	} else {
		perror();
		exit(1);
	}
}

struct file_list
{
	char name[256];
	struct file_list* next;
};

static void add_file_list(struct file_list** head, const char* name)
{
	struct file_list* l = malloc(sizeof(struct file_list));
	memset(l, 0, sizeof(struct file_list));
	strncpy(l->name, name, 255);
	l->next = NULL;

	struct file_list* list = *head;

	if(!*head)
		*head = l;
	else {
		while(list->next)
			list = list->next;

		list->next = l;
	}
}

static int len_file_list(struct file_list* head)
{
	if(!head) return 0;

	int x = 0;
	while(head)
	{
		x++;
		head = head->next;
	}

	return x;
}

static void dd(const char* input_file, const char* output_file,
		size_t block_size, size_t block_count, off_t seek)
{
	int pid = fork();

	if(pid > 0)
	{
		int status;
		if(waitpid(pid, &status, 0) < 0)
			err("waitpid failure.");

		if(WEXITSTATUS(status) != 0)
			err("child exec failure (objcopy).");

	} else if(pid == 0)
	{
		char _if[64];
		char _of[64];
		char _seek[64];
		char _count[64];
		char _bs[64];

		snprintf(_if, 64, "if=%s", input_file);
		snprintf(_of, 64, "of=%s", output_file);
		snprintf(_seek, 64, "seek=%d", (int)seek);
		snprintf(_count, 64, "count=%d", (int)block_count);
		snprintf(_bs, 64, "bs=%d", (int)block_size);

		char* args[8];
		args[0] = "dd";
		args[1] = _if;
		args[2] = _of;
		args[3] = _seek;
		args[4] = _count;
		args[5] = _bs;	
		args[6] = "conv=notrunc";
		args[7] = NULL;

		print_command(args);
		execvp(args[0], args);

		perror();
		exit(1);
	} else {
		perror();
		exit(1);
	}	
}

static void ld(struct file_list* input, const char* output)
{
	int pid = fork();

	if(pid > 0)
	{
		int status;
		if(waitpid(pid, &status, 0) < 0)
			err("waitpid failure.");

		if(WEXITSTATUS(status) != 0)
			err("Child exec failure (objcopy).");
	} else if(pid == 0)
	{
		int len = len_file_list(input);

		char text_start[128];
		char data_start[128];
		char rodata_start[128];
		char bss_start[128];
		char eh_frame_start[128];

		snprintf(text_start, 128, "--section-start=.text=0x%x", BOOT2_TEXT_S);
		snprintf(data_start, 128, "--section-start=.data=0x%x", BOOT2_DATA_S);
		snprintf(rodata_start, 128, "--section-start=.rodata=0x%x", BOOT2_RODATA_S);
		snprintf(bss_start, 128, "--section-start=.bss=0x%x", BOOT2_BSS_S);
		/* This will suppress warnings, but isn't used. */
		snprintf(eh_frame_start, 128, "--section-start=.eh_frame=0x%x", 
				PGROUNDUP(BOOT2_BSS_E));

		char* args[len + 10];

		char* cross_ld = getenv("CROSS_LD");

		if(!cross_ld)
		{
			printf("Please define CROSS_LD.\n");
			exit(1);
		}

		args[0] = cross_ld;
		args[1] = "--entry=main";
		args[2] = text_start;
		args[3] = data_start;
		args[4] = rodata_start;
		args[5] = bss_start;
		args[6] = eh_frame_start;
		args[7] = "-o";
		args[8] = (char*)output;

		int x;
		for(x = 0; x < len;x++)
		{
			assert(input);
			args[9 + x] = input->name;
			input = input->next;
		}

		args[9 + x] = NULL;

		print_command(args);
		execvp(args[0], args);

	} else {
		perror();
		exit(1);
	}
}

int main(int argc, char** argv)
{
	struct file_list* in_files = NULL;
	const char* out_file = NULL;
	const char* int_file = "boot-stage2.o";

	int x;
	for(x = 1;x < argc;x++)
	{
		if(!strcmp(argv[x], "-o"))
		{
			if(out_file)
				err("Output file specified twice.");
			out_file = argv[x + 1];
			x++;
		} else {
			add_file_list(&in_files, argv[x]);
		}
	}

	if(!out_file)
		err("You must specify an output file with -o.");
	if(!in_files)
		err("You must specify an input file.");

	size_t position = 0x0000;
	size_t start = BOOT2_TEXT_S;

	size_t text_sz = PGROUNDUP(BOOT2_TEXT_E - BOOT2_TEXT_S);
	size_t bss_sz = PGROUNDUP(BOOT2_BSS_E - BOOT2_BSS_S);
	size_t data_sz = PGROUNDUP(BOOT2_DATA_E - BOOT2_DATA_S);
	size_t rodata_sz = PGROUNDUP(BOOT2_RODATA_E - BOOT2_RODATA_S);
	size_t total_sz = PGROUNDUP(BOOT2_E - BOOT2_S);

	int block_size = 512;
	dd("/dev/zero", out_file, block_size, total_sz / block_size, 0);

	ld(in_files, int_file);

	create_section(int_file, ".text");
	create_section(int_file, ".data");
	create_section(int_file, ".rodata");

	char text_section[64];
	char data_section[64];
	char rodata_section[64];

	snprintf(text_section, 64, "%s.text", int_file);
	snprintf(data_section, 64, "%s.data", int_file);
	snprintf(rodata_section, 64, "%s.rodata", int_file);

	dd(text_section, out_file, block_size, text_sz / block_size, position / block_size);
	position += text_sz;
	dd(rodata_section, out_file, block_size, rodata_sz / block_size, position / block_size);
	position += rodata_sz;
	dd(data_section, out_file, block_size, data_sz / block_size, position / block_size);
	position += data_sz;
	position += bss_sz; /* BSS is zeroed when it starts*/

	return 0;
}
