#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"
#define OP_PIPE 0x01 /* \ */
#define OP_FILE 0x02 /* > */
#define OP_APPEND 0x03 /* >> */
#define OP_BACK 0x04 /* & */
#define OP_FILE_IN 0x05 /* < */

void runprog(char* string);
int main(int argc, char** argv)
{
	char in_buff[2048];
	while(1){
		printf("# ");
		fgets(in_buff, 2048, 0);
		int cur_cmd;
		int cur_op;
		cur_op = 0;
		cur_cmd = 1;
		char *cmd_buff[16];
		char op_buff[15];
		for(i=0;i<strlen(in_buff); i++){
			switch(in_buff[i])
			{
				default: break;
				case '|': 
				in_buff[i] = 0;
				op_buff[cur_op] = OP_PIPE;
				cur_op++;
				cmd_buff[cur_cmd] = in_buff+i+1;
				cur_cmd++;
				break;
				case '<':
				in_buff[i] = 0;
				op_buff[cur_op] = OP_FILE_IN;
				cur_op++;
				cmd_buff[cur_cmd] = in_buff+i+1;
				cur_cmd++;
				break;
				case '>':
				in_buff[i] = 0;
				op_buff[cur_op] = OP_FILE;
				if(in_buff[i+1] == '>'){
					op_buff[cur_op] = OP_APPEND;
					i++;
					in_buff[i] = 0;
				}
				cur_op++;
				cmd_buff[cur_cmd] = in_buff+i+1;
				cur_cmd++;
				break;
				case '&':
				in_buff[i] = 0;
				op_buff[cur_op] = OP_BACK;
				cur_op++;
				cmd_buff[cur_cmd] = in_buff+i+1;
				cur_cmd++;
				break;
			}
			
		}
		int fds[15];
		int fds_write[15];
		char error = 0;
		for(i = 0; i<16 && error == 0; i++){
			if(cmd_buff[i] == 0 && op_buff[i-1]!=0){
				printf("sh: invalid op\n");
				break;
			}
			if(cmd_buff[i]==0){
				break;
			}
			char* cmd = cmd_buff[i];
			int fd_file ;
			int fd_curr[2]; // 0 read 1 write
			
			
			
			switch(op_buff[i]){
				default: 
				printf("sh: invalid op\n");
				error = 1;
				break;
				case OP_PIPE:
				pipe(fd_curr);
				//dup2(1, fd_curr[1]);
				fds[i] = fd_curr[0];
				fds_write[i] = fd_curr[1];
				break;
				case OP_FILE:
				fd_file = open(cmd_buff[i+1], O_CREATE|O_WRONLY|O_TRUC, 0x0);
				//dup2(1, fd_file);
				if(op_buff[i+1]!=0){
					printf("sh: invalid op on file\n");
					error = 1;
				}
				break;
				case OP_APPEND:
				fd_file = open(cmd_buff[i+1], O_CREATE|O_WRONLY, 0x0);
				lseek(fd_file, 0, SEEK_END);
				//dup2(1, fd_file);
				if(op_buff[i+1]!=0){
					printf("sh: invalid op on file\n");
					error = 1;
				}
				break;
				case OP_FILE_IN:
				if(i!=0){
					printf("sh: invalid op\n");
					error = 1;
					break;
				}	
				fd_file = open(cmd_buff[i+1], O_RDONLY, 0x0);
				if(fd_file == -1){
					printf("sh: no such file %s\n", cmd_buff[i+1]);
					error = 1;
					break;
				}
				//dup2(0, fd_file);
				break;	
				/*Have not dealt with the background op*/
			}
			if(i>0){
				switch(op_buff[i-1]){
					default:
					printf("sh: invalid op\n");
					error = 1;
					break;
					case OP_PIPE:
					//dup2(0, fds[i-1]);
					break;
				}	
			}
			int f = fork();
			if(f==0){//is child
				switch(op_buff[i]){
					case OP_PIPE:
					dup2(1, fd_curr[1]);
					break;
					case OP_FILE:
					dup2(1, fd_file);
					break;
					case OP_APPEND:
					dup2(1, fd_file);
					break;
					case OP_FILE_IN:
					dup2(0, fd_file);
					break;	
					/*Have not dealt with the background op*/
				}
				if(i>0){
					switch(op_buff[i-1]){
						case OP_PIPE:
						dup2(0, fds[i-1]);
						break;
					}	
				}
			runprog(cmd1);	
			
			}
			
		}
	
	}
	return 0;
	
}



void runprog(char* string){
	int i;
	int arg = 1;
	char* argv[64];
	argv[0] = in_buff;
	for(i = 0; i<strlen(in_buff); i++ ){
		if(in_buff[i]==' '){
			in_buff[i] = 0;
			argv[arg] = in_buff + i + 1;
			arg++;	
		}
	}
	exec(argv[0], argv);
}
