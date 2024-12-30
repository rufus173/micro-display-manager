#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>

struct desktop {
	char *display_name;
	char *start_command;
	char *compositor;
}

static int desktop_count = 0;
static desktop **available_desktops = NULL;

static int load_from_dir(char *dir,char *compositor){
	DIR *desktop_sessions_dir = opendir(dir);
	if (desktop_sessions_dir == NULL){
		perror("opendir");
		return -1
	}
	struct dirent *file = readdir(desktop_sessions_dir);
	//each file in the dir
	for (;file != NULL;file = readdir(desktop_sessions_dir)){
		printf("%s\n",file.d_name);
		FILE *desktop = fopen(file.d_name);
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
	for (int i = 0; i < desktop_count; i++) free(available_desktops[i]);
	free(available_desktops);
}
