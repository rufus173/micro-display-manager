#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <pwd.h>

struct desktop {
	char *display_name;
	char *start_command;
	char *compositor;
};

static int desktop_count = 0;
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
int start_desktop(int desktop_index,char *user){
	if (strcmp(get_desktop_compositor(desktop_index),"wayland") == 0){
		//we just run the start command, and nothing else
		char *user_shell = getpwnam(user)->pw_shell;
		execl(user_shell,basename(user_shell),"-lc",get_desktop_start_command(desktop_index),NULL);
	}
	fprintf(stderr,"unrecognised desktop compositor.\n");
	//this function should not return unless an error occured
	return -1;
}
