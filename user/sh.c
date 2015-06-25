#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"


int main(int argc, char** argv)
{
	char in_buff[128];
	while(1){
		printf("# ");
		fgets(in_buff, 128, 0);
		int f = fork();
		if(f>0){
			wait(f);
		}else{
			int i;
			int arg = 1;
			char* argv[64];
			argv[0] = in_buff;
			for(i = 0; i<strlen(in_buff) i++ ){
				if(in_buff[i]==' '){
					in_buff[i] = 0;
					argv[arg] = in_buff + i + 1;
					arg++;
				}
			}
			argv[arg] = NULL;
			exec(argv[0], argv);
		}
	}
	
	return 0;
}
