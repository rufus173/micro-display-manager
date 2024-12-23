#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pam.h"
#include <wait.h>
#include "graphical-interface.h"
int main(int argc, char **argv){

	//start user interface
	start_gui(argc,argv);

	//arg processing
	if (argc < 3){
		fprintf(stderr,"requires 2 args of user and password\n");
		return 1;
	}

	//variables
	char *username = argv[1];
	char *password = argv[2];


	//login with pam
	int result = login(username,password,NULL);
	if (result < 0){
		fprintf(stderr,"login failed.\n");
		return 1;
	}
	
	//start a shell
	pid_t child_pid = fork();
	if (child_pid < 0){
		perror("fork");
		return 1;
	}
	if (child_pid == 0){
		//child process
		execl("/bin/bash","--login",NULL);
	}
	result = waitpid(child_pid,NULL,0);
	if (result < 0){
		perror("waitpid");
		return 1;
	}
	
	//logout
	logout(NULL); //dont care about result

	//do stuff
	return 0;
}
