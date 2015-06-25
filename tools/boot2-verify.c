#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define TEXT_MAX (512 * 58)
#define DATA_MAX (512 * 2)
#define RODATA_MAX (512 * 2)
#define BSS_MAX (512 * 2)

int main(int argc, char** argv)
{
	printf("*** Boot stage 2 verification ***\n");

	int text = open("kernel/boot/boot-stage2.text", O_RDONLY);
	int text_end = lseek(text, 0, SEEK_END);
	int data = open("kernel/boot/boot-stage2.data", O_RDONLY);
	int data_end = lseek(data, 0, SEEK_END);
	int rodata = open("kernel/boot/boot-stage2.rodata", O_RDONLY);
	int rodata_end = lseek(rodata, 0, SEEK_END);
	int bss = open("kernel/boot/boot-stage2.bss", O_RDONLY);
	int bss_end = lseek(bss, 0, SEEK_END);
	
	if(text < 0 || data < 0 || rodata < 0 || bss < 0)
	{
		printf("FATAL ERROR: problem with program segment.\n");
		char* ok = "ok";
		char* bad = "bad";
		printf("text: %s data: %s rodata: %s bss: %s\n",
			text > 0 ? ok : bad, 
			data > 0 ? ok : bad,
			rodata > 0 ? ok : bad,
			bss > 0 ? ok : bad);
		exit(1);
	}

	if(text_end > TEXT_MAX)
	{
		printf("FATAL ERROR: Boot stage 2\n");
		printf("text section too large: %d\n", text_end);
		exit(1);
	}

	if(data_end > DATA_MAX)
	{
		printf("FATAL ERROR: Boot stage 2\n");
		printf("data section too large: %d\n", data_end);
		exit(1);
	}

	if(rodata_end > DATA_MAX)
	{
		printf("FATAL ERROR: Boot stage 2\n");
		printf("rodata section too large: %d\n", rodata_end);
		exit(1);
	}

	if(bss_end > DATA_MAX)
	{
		printf("FATAL ERROR: Boot stage 2\n");
		printf("bss section too large: %d\n", bss_end);
		exit(1);
	}

	close(text);
	close(data);
	close(rodata);
	close(bss);

	printf("Boot stage 2 has been verified.\n");

	return 0;
}
