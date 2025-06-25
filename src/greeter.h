#ifndef _GREETER_H
#define _GREETER_H
#include "start_commands.h"
//return a handle that is passed to the other functions
void *greeter_init();
int greeter_end(void *handle);
int greeter_get_login_info(void *handle,char **user, char **password, int *desktop_index);
int greeter_show_error(void *handle,const char *messsage);
int greeter_show_info(void *handle,const char *message);
#endif
