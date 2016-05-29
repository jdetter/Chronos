#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <dirent.h>

#define __LINUX__
#include "file.h"

#define OP_PIPE 0x01 /* \ */
#define OP_FILE 0x02 /* > */
#define OP_APPEND 0x03 /* >> */
#define OP_BACK 0x04 /* & */
#define OP_FILE_IN 0x05 /* < */

#define MAX_CMD 16

int trim(char* str)
{
        int x;
        for(x = 0;x < strlen(str);x++)
                if(str[x] != ' ')break;
        strncpy(str, str + x, strlen(str) + 1);
        for(x = strlen(str) - 1;x >= 0;x--)
                if(str[x] != ' ') break;
        str[x + 1] = 0;
        return strlen(str);
}

extern char** environ;

void runprog(char* string);
int main(int argc, char** argv)
{
	/* We need to set the TERM environment variable */
	putenv("TERM=xterm-color");
	putenv("C_INCLUDE_PATH=/usr/include");
	putenv("CPLUS_INCLUDE_PATH=/usr/include");
	putenv("PATH=/bin:/usr/bin:/usr/i686-pc-chronos/bin");

	char in_buff[2048];
	while(1){
		memset(in_buff, 0, 2048);
		printf("# ");
		fflush(0);
		fgets(in_buff, 2048, stdin);
		/* delete new line character */
		in_buff[strlen(in_buff) - 1] = 0;
		trim(in_buff);
		if(strlen(in_buff) == 0) continue;

		/* check for cd */
		if(!memcmp(in_buff, "cd ", 3))
		{
			/* do cd and reset */
			runprog(in_buff);
			continue;
		}
		/* Check for exit */
		if(!memcmp(in_buff, "exit", 4) &&
			strlen(in_buff) == 4)
		{
			exit(0);
		}
		int cur_cmd;
		int cur_op;
		cur_op = 0;
		cur_cmd = 1;
		
		char *cmd_buff[MAX_CMD];
		memset(cmd_buff, 0, sizeof(char*) * 16);
		*cmd_buff = in_buff;
		char op_buff[MAX_CMD - 1];
		memset(op_buff, 0, MAX_CMD - 1);
		int i;
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

		for(i = 0;i < MAX_CMD;i++)
		{
			if(cmd_buff[i]) trim(cmd_buff[i]);
		}

		int fds[MAX_CMD - 1];
		memset(fds, 0, sizeof(int) * (MAX_CMD - 1));
		int fds_write[MAX_CMD - 1];
		memset(fds_write, 0, sizeof(int) * (MAX_CMD - 1));
		int pids[MAX_CMD];
		memset(pids, 0, sizeof(int) * (MAX_CMD - 1));
		char error = 0;
		if(cmd_buff[0] == 0)
			continue;
		for(i = 0; i < MAX_CMD && error == 0; i++){
			if(i > 0 && cmd_buff[i] == 0 && op_buff[i-1] != 0){
				printf("sh: invalid op\n");
				break;
			}
			if(cmd_buff[i]==0){
				break;
			}
			char* cmd = cmd_buff[i];
			int fd_file ;
			int execable = 1;
			int fd_curr[2]; // 0 read 1 write
			
			switch(op_buff[i]){
				case 0x0: break;
				default:
				printf("sh: invalid op\n");
				error = 1;
				break;
				case OP_PIPE:
				pipe(fd_curr);
				fds[i] = fd_curr[0];
				fds_write[i] = fd_curr[1];
				break;
				case OP_FILE:
				fd_file = open(cmd_buff[i+1], 
					O_CREAT|O_WRONLY | O_TRUNC,
					PERM_ARD | PERM_GWR | PERM_UWR);
				if(op_buff[i+1]!=0){
					printf("sh: invalid op on file\n");
					error = 1;
				}
				break;
				case OP_APPEND:
				fd_file = open(cmd_buff[i+1], 
					O_CREAT|O_WRONLY, 
					PERM_ARD | PERM_GWR | PERM_UWR);
				lseek(fd_file, 0, SEEK_END);
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
				break;	
				/*Have not dealt with the background op*/
			}
			if(i > 0)
			{
				switch(op_buff[i-1]){
					default:
					printf("sh: invalid op\n");
					error = 1;
					break;
					case OP_PIPE:
					break;
					case OP_APPEND:
					execable = 0;
					break;
					case OP_FILE:
					execable = 0;
					break;
				}	
			}

			if(!execable) break;
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
				if(i > 0){
					switch(op_buff[i-1]){
						case OP_PIPE:
						dup2(0, fds[i-1]);
						break;
					}	
				}
				runprog(cmd);
				exit(1);
			} else pids[i] = f;
		}

		for(i = 0;i < MAX_CMD;i++)
		{
			if(pids[i] <= 0) break;
			waitpid(pids[i], NULL, 0);
		}

		for(i = 0;i < MAX_CMD;i++)
                {
                        if(fds_write[i] <= 0) break;
                        close(fds_write[i]);
                }

		for(i = 0;i < MAX_CMD;i++)
                {
                        if(fds[i] <= 0) break;
                        close(fds[i]);
                }	
	}
	return 0;
	
}

void runprog(char* string){
	trim(string);
	int i;
	int length = strlen(string);
	int arg = 1;
	char* argv[64];
	memset(argv, 0, 64 * sizeof(char*));
	argv[0] = string;
	int spaces = 0;
	for(i = 0; i<length; i++ ){
		if(string[i]==' ' && !spaces){
			spaces ++;
			string[i] = 0;
			argv[arg] = string + i + 1;
			arg++;	
		} else spaces = 0;
	}
	argv[63] = 0;

	/* Trim all */
	for(i = 0;i < 63;i++)
	{
		if(!argv[i]) break;
		else trim(argv[i]);
	}
	if(!argv[0]) return;

	/* Check for builtin */
	if(!strcmp(argv[0], "cd"))
	{
		if(!argv[1]) return;
		/* Change directory */
		if(chdir(argv[1]))
		{
			printf("sh: %s: permission denied.\n", argv[1]);
			return;
		}

		char cwd_buff[128];
		getcwd(cwd_buff, 128);
	} else {
		execvp(argv[0], (char* const*)argv);
		printf("sh: binary not found: %s\n", argv[0]);
	}

	return;
}
