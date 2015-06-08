#include "types.h"
#include "stdarg.h"
#include "stdlib.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char** argv)
{
	if(argc<2)
	{
		//printf("Usage: cat [-n] [file]/n");
		exit();
	}
	char* path= NULL;
	//int printingNumbers = 0;
	int lineNum = 0;
	if(strcmp(argv[1], "-n")==0)
	{
		//printingNumbers = 1;
		path = argv[2];
	}
	else
	{
		path = argv[1];
	}
	int fileDescript = open(path);
	if(fileDescript==-1)
	{
		//printf("File does not exist.");
		exit();
	}
	//int fileLength = lseek(fileDescript, 0, FSEEK_END);
	char buffer[0x1000];
	//int i;
	//for(i=0; i<fileLength; i+=0x1000)
	{
		read(fileDescript, buffer, 0x1000);
		//printf(buffer);

		//for(i=0; i<fileLength; i++)
		{
			//if(buffer[i]== '/n')
			{
				//printf(lineNum);
				lineNum++;
			}
		}
	}

	exit();
}
