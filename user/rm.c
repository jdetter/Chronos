#include "chronos.h"
#include "stdio.h"

int rec_rmdir(char* dir){

	char res_path[FILE_MAX_PATH];	
	file_path_dir(dir, res_path, FILE_MAX_PATH);
	int dirfile = open(res_path, O_RDWR, 0x0);

	if(dirfile == -1){
		return -1;
	}
	struct directent dir_stuff;	
	int ind;
	for(ind = 0; readdir(dirfile, ind, &dir_stuff) != -1; ind++){
		if((dir_stuff.type == FILE_TYPE_FILE) || (dir_stuff == FILE_TYPE_SPECIAL)
			|| (dir_stuff == FILE_TYPE_LINK)){
			char file_path[FILE_MAX_PATH];
			strncpy(file_path, res_path, FILE_MAX_PATH);
			strncat(file_path, dir_stuff.name, FILE_MAX_PATH);
			rm(file_path);
		}
		else if(dir_stuff.type == FILE_TYPE_DIR){
			char dir_path[FILE_MAX_PATH];
			strncpy(dir_path, res_path, FILE_MAX_PATH);
			strncat(dir_path, dir_stuff.name, FILE_MAX_PATH);
			rec_rmdir(dir_path);
		}
	}
	int rmres = rmdir(res_path);
	if(rmres == -1){
		return -1;
	}
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
	exit();
}

int main(int argc, char** argv)
{
	if(argc < 2){
		return -1;
	}	
	int direct_rm = 0;
	int user_input = 1;
	int interactive = 1;
	int recursive = 0;
	int print = 0;

	int i = 1;
	for(i = 1; argv[i] != NULL; i++){

		if(*argv[i] == '-'){
			int k = 1;
			char* flags = argv[i];
			for(;flags[k] != NULL; k++){			

				switch(options[i]){
			
					case 'd':
						direct_rm = 1;
						break;
					case 'f':
						user_input = 0;
						break;
					case 'i':
						interactive = 1;
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
	int response;
	for(; argv[j] != NULL; j++){
		if(*argv[j] != '-'){
			int fd = open(argv[j], O_RDWR, 0);
			struct file_stat file_dir; 
			fstat(fd, &file_dir);
			int file_type = file_dir.type;

			if(file_type == FILE_TYPE_FILE || file_type == FILE_TYPE_SPECIAL ||
				file_type == FILE_TYPE_LINK){
				int removed = rm(argv[j]);
				if(removed == -1){
					if(user_input){
						printf("rm: could not remove file %s\n", argv[j]);
						exit();
					}
				}
				if(print){
					printf("rm: %s was removed!\n", argv[j]);
				}	
			}
			else if(file_type == FILE_TYPE_DIR){
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
							exit();
						}
					}
					if(print){
						printf("rm: %s was removed!\n", argv[j]);
					}
				
				}
				else{
					printf("rm: -d or -r must be enabled to delete directories\n");
					exit();
				}
			}
		}
	}
	exit();
}
