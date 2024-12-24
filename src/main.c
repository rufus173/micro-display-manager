#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pam.h"
#include <wait.h>
#include "graphical-interface.h"
int start_new_session(char *username, char *password);
int main(int argc, char **argv){

	//calls with args username and password when login is pressed
	register_login_button_function(start_new_session);

	//start user interface
	start_gui(argc,argv);

	//start a shell
	pid_t child_pid = fork();
	if (child_pid < 0){
		perror("fork");
		return -1;
	}
	if (child_pid == 0){
		//child process
		//execl("/bin/bash","--login",NULL);
		sleep(10);
		exit(0);
	}
	int result = waitpid(child_pid,NULL,0);
	if (result < 0){
		perror("waitpid");
		return -1;
	}
	
	//logout
	logout(NULL); //dont care about result
}
int start_new_session(char *username, char *password){
	//login with pam
	int result = login(username,password,NULL);
	if (result < 0){
		show_message_popup("Incorrect username or password.");
		fprintf(stderr,"login failed.\n");
		return -1;
		//interface stays open to try again
	}

	return 0; //closes graphical interface and continues in main func 
}
