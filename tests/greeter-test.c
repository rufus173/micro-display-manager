#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/greeter.h"
#include "../src/start_commands.h"
int main(int argc, char **argv){
	struct available_desktops *desktops = load_desktops();
	char *user, *password;
	int desktop_index;
	void *state = greeter_init(desktops);
	fprintf(stderr,"funciton: sample perror output\n");
	greeter_get_login_info(state,&user,&password,&desktop_index);
	greeter_show_error(state,"this is an incredibly long error message that i typed myself");
	greeter_end(state);
	printf("username: [%s]\npassword: [%s]\ndesktop: [%s]\n",user,password,get_desktop_name(desktops,desktop_index));
	free(user);
	free(password);
	free_desktops(desktops);
}
