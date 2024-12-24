#ifndef _GRAPHICAL_INTERFACE_H
#define _GRAPHICAL_INTERFACE_H
int start_gui(int argc, char **argv);
void show_message_popup(char *);
void register_login_button_function(int (*registered_function)(char *,char *));
#endif
