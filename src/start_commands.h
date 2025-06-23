#ifndef _START_COMMANDS_H
#define _START_COMMANDS_H
int load_desktops(); //returns the count
void free_desktops();
int get_desktop_count();
char *get_desktop_name(int index);
char *get_desktop_start_command(int index);
int start_desktop(int index, char *user);
int set_last_selected_desktop_index(int index);
int get_last_selected_desktop_index();
int wait_display_server();
int mkdir_p(const char *path);
struct desktop {
	char *display_name;
	char *start_command;
	char *compositor;
};
#endif
