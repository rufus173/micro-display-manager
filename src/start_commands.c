#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <pwd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define DESKTOP_CACHE_FOLDER "/var/cache/microdm"
#define DESKTOP_CACHE_FILE DESKTOP_CACHE_FOLDER "/last_desktop"

struct desktop {
	char *display_name;
	char *start_command;
	char *compositor;
};
struct available_desktops {
	int desktop_count;
	struct desktop **available_desktops;
};

static pid_t display_server_pid = 0;

int mkdir_p(const char *path){
	int result = mkdir(path,0777);
	if (result < 0){
		if (errno == ENOENT){
			char *base_path = strdup(path);
			if (strrchr(base_path,'/') == NULL) return -1;
			*strrchr(base_path,'/') = '\0';
			int result = mkdir_p(base_path);
			free(base_path);
			if (result == 0){
				return mkdir_p(path);
			}else{
				return result;
			}
		}else{
			return -1;
		}
	}
	return 0;
}

static char *file_get_line(FILE *file){
	if (feof(file)) return NULL;

	char *line = malloc(1);
	int line_size = 1;
	line[0] = '\0';
	for (;;){
		char buffer[1];
		int result = fread(buffer,1,1,file);
		if (result < 1){
			if (feof(file)) break;
			if (ferror(file) != 0){
				perror("fread");
				free(line);
				return NULL;
			}
		}

		//stop when we hit a newline
		if (buffer[0] == '\n') break;

		line = realloc(line,line_size+1);
		line[line_size-1] = buffer[0];
		line[line_size] = '\0';
		line_size++;
	}
	return line;
}
static char *config_file_read_attribute(FILE *file,char *attribute){
	fseek(file,0,SEEK_SET); //go to begining of file
	char *line;
	for (;;){
		line = file_get_line(file);
		if (line == NULL) break; //at the end of the file

		int search_term_size = strlen(attribute) + 1 + 1; // = and '\0'
		char *search_term = malloc(search_term_size);
		snprintf(search_term,search_term_size,"%s=",attribute);
		if (strncmp(line,search_term,strlen(search_term)) == 0){
			free(search_term);
			char *value = strdup(strchr(line,'=')+1);
			free(line);
			return value;
		}
		free(search_term);
		free(line);
	}
	return NULL; //no results
}
static int load_from_dir(struct available_desktops *desktops,char *dir,char *compositor){
	DIR *desktop_sessions_dir = opendir(dir);
	if (desktop_sessions_dir == NULL){
		perror("opendir");
		return -1;
	}
	struct dirent *file = readdir(desktop_sessions_dir);
	//each file in the dir
	for (;file != NULL;file = readdir(desktop_sessions_dir)){// '/' + '\0'

		if (file->d_type != DT_REG) continue;//weed out the .. and . direcotries
		int file_path_size = strlen(dir)+strlen(file->d_name)+1+1;
		char *file_path = malloc(file_path_size);
		snprintf(file_path,file_path_size,"%s/%s",dir,file->d_name);
		
		FILE *desktop = fopen(file_path,"r");
		free(file_path);

		if (desktop == NULL){
			perror("fopen");
			closedir(desktop_sessions_dir);
			return -1;
		}
		//initialise struct
		desktops->desktop_count++;
		desktops->available_desktops = realloc(desktops->available_desktops,sizeof(struct desktop*)*desktops->desktop_count);
		struct desktop *entry = malloc(sizeof(struct desktop));
		desktops->available_desktops[desktops->desktop_count-1] = entry;

		//set compositor
		entry->compositor = compositor;
		entry->display_name = config_file_read_attribute(desktop,"Name");
		entry->start_command = config_file_read_attribute(desktop,"Exec");


		//printf("%s: %s\n",entry->display_name,entry->start_command);

		fclose(desktop);
	}
	closedir(desktop_sessions_dir);
	return 0;
}

struct available_desktops *load_desktops(){
	struct available_desktops *desktops = malloc(sizeof(struct available_desktops));
	memset(desktops,0,sizeof(struct available_desktops));
	//load wayland sesisons
	int result = load_from_dir(desktops,"/usr/share/wayland-sessions","wayland");
	if (result < 0) return NULL;
	return desktops;
}

void free_desktops(struct available_desktops *desktops){
	for (int i = 0; i < desktops->desktop_count; i++){
		free(desktops->available_desktops[i]->display_name);
		free(desktops->available_desktops[i]->start_command);
		free(desktops->available_desktops[i]);
	}
	free(desktops->available_desktops);
	free(desktops);
}
int get_desktop_count(struct available_desktops *desktops){
	return desktops->desktop_count;
}
char *get_desktop_name(struct available_desktops *desktops,int index){
	return desktops->available_desktops[index]->display_name;
}
char *get_desktop_start_command(struct available_desktops *desktops,int index){
	return desktops->available_desktops[index]->start_command;
}
char *get_desktop_compositor(struct available_desktops *desktops,int index){
	return desktops->available_desktops[index]->compositor;
}
int get_last_selected_desktop_index(struct available_desktops *desktops){
	FILE *last_desktop_file = fopen(DESKTOP_CACHE_FILE,"rb");
	if  (last_desktop_file == NULL){
		fprintf(stderr,"couldnt open cache.\n");
		return 0; //default to 0
	}
	uint32_t last_desktop_index = 0;
	int result = fread(&last_desktop_index,sizeof(uint32_t),1,last_desktop_file);
	if (result < 1){ //error if we couldnt read the whole int or just couldnt read
		fprintf(stderr,"couldnt get last desktop used");
		last_desktop_index = 0;
	}
	fclose(last_desktop_file);
	return last_desktop_index;
}
int set_last_selected_desktop_index(struct available_desktops *desktops,int index){
	mkdir_p(DESKTOP_CACHE_FOLDER);
	FILE *cache = fopen(DESKTOP_CACHE_FILE,"wb");
	if  (cache == NULL){
		fprintf(stderr,"couldnt open cache.\n");
		return -1;
	}
	uint32_t index_buffer = index;
	fwrite(&index_buffer,sizeof(uint32_t),1,cache);
	fclose(cache);
	return 0;
}
int start_desktop(struct available_desktops *desktops,int desktop_index,char *user){
	//switch to users home directory
	chdir(getpwnam(user)->pw_dir);
	char *user_shell = getpwnam(user)->pw_shell;
	//=================== wayland sessions ================
	if (strcmp(get_desktop_compositor(desktops,desktop_index),"wayland") == 0){
		//we just run the start command, and nothing else
		printf("%s %s %s %s\n",user_shell,basename(user_shell),"-lc",get_desktop_start_command(desktops,desktop_index));
		execl(user_shell,basename(user_shell),"-lc",get_desktop_start_command(desktops,desktop_index),NULL);
	}
	//this function should not return unless an error occured
	fprintf(stderr,"unrecognised desktop compositor.\n");
	return -1;
}
int wait_display_server(){
	int result = waitpid(display_server_pid,NULL,0);
	if (result < 0){
		perror("waitpid");
		return -1;
	}
	return 0;
}
