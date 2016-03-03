#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define __LINUX__
#define __FILE_NO_FUNC__
#include "file.h"
#include <dirent.h>

int file_path_dir(const char* src, char* dst, uint sz)
{
        if(strlen(src) + 1 >= sz)
                return -1;
        memset(dst, 0, sz);
        int len = strlen(strncpy(dst, src, sz));
        if(dst[len - 1] != '/')
                dst[len] = '/';
        return 0;
}

int file_path_file(const char* src, char* dst, uint sz)
{
        if(strlen(src) == 0) return 1;
        memset(dst, 0, sz);
        int len = strlen(strncpy(dst, src, sz));
        if(!strcmp(dst, "/")) return 0;
        for(;dst[len  - 1] == '/' && len - 1 >= 0;len--)
                dst[len - 1] = 0;
        return 0;
}


int rec_rmdir(char* dir){

	char res_path[FILE_MAX_PATH];	
	file_path_dir(dir, res_path, FILE_MAX_PATH);
	int dirfile = open(res_path, O_RDWR, 0x0);

	if(dirfile == -1){
		return -1;
	}
	struct dirent dir_stuff;	
	int ind;
	for(ind = 0; getdents(dirfile, &dir_stuff, 1) != 0; ind++){
		struct stat st;
		fstat(dirfile, &st);
		if((S_ISREG(st.st_mode)) || (S_ISLNK(st.st_mode))){
			char file_path[FILE_MAX_PATH];
			strncpy(file_path, res_path, FILE_MAX_PATH);
			strncat(file_path, dir_stuff.d_name, FILE_MAX_PATH);
			unlink(file_path);
		}
		else if(S_ISDIR(st.st_mode)){
			char dir_path[FILE_MAX_PATH];
			strncpy(dir_path, res_path, FILE_MAX_PATH);
			strncat(dir_path, dir_stuff.d_name, FILE_MAX_PATH);
			rec_rmdir(dir_path);
		}
	}
	int rmres = rmdir(res_path);
	if(rmres == -1){
		return -1;
	}
	close(dirfile);
	return 0;
}

int confirm_delete(){
	
	char uinput[75];
	printf("Are you sure you want to delete?");
	fgets(uinput, 75, 0x0);
	if(strcmp(uinput, "y")){
		return 1;
	}	
	else{
		return 0;
	}
}

void usage(){
	printf("Usage: rm [-dfiRrv] file1, file2, ...");
	exit(1);
}

int main(int argc, char** argv)
{
	if(argc < 2){
		return -1;
	}	
	int direct_rm = 0;
	int user_input = 1;
	int recursive = 0;
	int print = 0;

	int i = 1;
	for(i = 1; argv[i] != NULL; i++){

		if(*argv[i] == '-'){
			int k = 1;
			char* flags = argv[i];
			for(;flags[k] != 0x0; k++){			

				switch(flags[k]){
			
					case 'd':
						direct_rm = 1;
						break;
					case 'f':
						user_input = 0;
						break;
					case 'v':
						print = 1;
						break;
					case 'R':
					case 'r':
						recursive = 1;
						break;
					default:
						usage();
						break;
				}
			}
		}
	}

	int j = 1;
	for(; argv[j] != NULL; j++){
		if(*argv[j] != '-'){
			int fd = open(argv[j], O_RDWR, 0);
			struct stat file_dir; 
			fstat(fd, &file_dir);

			if(S_ISREG(file_dir.st_mode) 
				|| S_ISLNK(file_dir.st_mode)){
				close(fd);
				int removed = unlink(argv[j]);
				if(removed == -1){
					if(user_input){
						printf("rm: could not remove file %s\n", argv[j]);
						return 1;
					}
				}
				if(print){
					printf("rm: %s was removed!\n", argv[j]);
				}	
			}
			else if(S_ISDIR(file_dir.st_mode)){
				if(direct_rm){
					int rmdirret;
					if(recursive){
						rmdirret = rec_rmdir(argv[j]);
					}
					else{
						rmdirret = rmdir(argv[j]);
					}
					if(rmdirret == -1){
						if(user_input){
							printf("rm: could not remove directory %s\n", argv[j]);
							return 1;
						}
					}
					if(print){
						printf("rm: %s was removed!\n", argv[j]);
					}
				
				}
				else{
					printf("rm: -d or -r must be enabled to delete directories\n");
					return 1;
				}
			}
		close(fd);
		}
	}
	return 0;
}
