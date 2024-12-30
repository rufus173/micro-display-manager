#ifndef _START_COMMANDS_H
#define _START_COMMANDS_H
int load_desktops(); //returns the count
void free_desktops();
int get_desktop_count();
char *get_desktop_name(int index);
char *get_desktop_start_command(int index);
#endif
