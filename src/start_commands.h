#ifndef _START_COMMANDS_H
#define _START_COMMANDS_H
#include "start_commands.h"
struct available_desktops *load_desktops(); //returns the count
void free_desktops(struct available_desktops *desktops);
int get_desktop_count(struct available_desktops *desktops);
char *get_desktop_name(struct available_desktops *desktops,int index);
char *get_desktop_start_command(struct available_desktops *desktops,int index);
int start_desktop(struct available_desktops *desktops,int index, char *user);
int set_last_selected_desktop_index(struct available_desktops *desktops,int index);
int get_last_selected_desktop_index(struct available_desktops *desktops);
int mkdir_p(const char *path);
struct desktop {
	char *display_name;
	char *start_command;
	char *compositor;
};
struct available_desktops {
	int desktop_count;
	struct desktop **available_desktops;
};
#endif
