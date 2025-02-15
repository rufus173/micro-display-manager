#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <pwd.h>
#include <stdint.h>
#include <sys/wait.h>

#define DESKTOP_CACHE_FILE "/var/cache/microdm/last_desktop"

struct desktop {
	char *display_name;
	char *start_command;
	char *compositor;
};

static int desktop_count = 0;
static pid_t display_server_pid = 0;
static struct desktop **available_desktops = NULL;

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
static int load_from_dir(char *dir,char *compositor){
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
		desktop_count++;
		available_desktops = realloc(available_desktops,sizeof(struct desktop*)*desktop_count);
		struct desktop *entry = malloc(sizeof(struct desktop));
		available_desktops[desktop_count-1] = entry;

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

int load_desktops(){
	//load xsessions form /usr/share/xsessions
	int result = load_from_dir("/usr/share/xsessions","X");
	if (result < 0) return -1;
	//load wayland sesisons
	result = load_from_dir("/usr/share/wayland-sessions","wayland");
	if (result < 0) return -1;

}

void free_desktops(){
	for (int i = 0; i < desktop_count; i++){
		free(available_desktops[i]->display_name);
		free(available_desktops[i]->start_command);
		free(available_desktops[i]);
	}
	free(available_desktops);
}
int get_desktop_count(){
	return desktop_count;
}
char *get_desktop_name(int index){
	return available_desktops[index]->display_name;
}
char *get_desktop_start_command(int index){
	return available_desktops[index]->start_command;
}
char *get_desktop_compositor(int index){
	return available_desktops[index]->compositor;
}
int get_last_selected_desktop_index(){
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
int set_last_selected_desktop_index(int index){
	FILE *cache = fopen(DESKTOP_CACHE_FILE,"wb");
	if  (cache == NULL){
		fprintf(stderr,"couldnt open cache.\n");
		fclose(cache);
		return -1;
	}
	uint32_t index_buffer = index;
	fwrite(&index_buffer,sizeof(uint32_t),1,cache);
	fclose(cache);
	return 0;
}
int start_desktop(int desktop_index,char *user){
	//switch to users home directory
	chdir(getpwnam(user)->pw_dir);
	char *user_shell = getpwnam(user)->pw_shell;
	//=================== wayland sessions ================
	if (strcmp(get_desktop_compositor(desktop_index),"wayland") == 0){
		//we just run the start command, and nothing else
		printf("%s %s %s %s\n",user_shell,basename(user_shell),"-lc",get_desktop_start_command(desktop_index));
		execl(user_shell,basename(user_shell),"-lc",get_desktop_start_command(desktop_index),NULL);
	}
	//=================== X sessions =====================
	else if (strcmp(get_desktop_compositor(desktop_index),"X") == 0){
		//start x server
		display_server_pid = fork();
		if (display_server_pid == 0){ //child
			execl("/usr/bin/X","X",NULL);
		}
		execl(user_shell,basename(user_shell),"-lc",get_desktop_start_command(desktop_index),NULL);
		//run desktop session start command
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
