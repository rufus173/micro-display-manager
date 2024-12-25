#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pam.h"
#include <wait.h>
#include "graphical-interface.h"
#include <pwd.h>
#include <grp.h>
#include "tui.h"
static void init_env(char *username);
static int set_ids(char *user);
int main(int argc, char **argv){
	if (getuid() != 0){
		fprintf(stderr,"Must be run as root.\n");
		return -1;
	}
	
	//start the tui
	int result = tui_init("/dev/tty1");
	if (result < 0){
		fprintf(stderr,"tui would not start.\n");
		return 1;
	}
	tui_end();

	if (argc < 3){
		fprintf(stderr,"not enough args. expected username and password\n");
		return 1;
	}

	char *user = argv[1];
	char *password = argv[2];

	//-------- mainloop to allow more then one login and logout ---------
	for (;;){
		//------------ check credentials with pam ------------
		printf("verifying credentials\n");
		int result = pam_login(user,password,NULL); //switch to desired user
		if (result < 0){
			fprintf(stderr,"could not log in\n");
			goto logout;
		}
		printf("pam login successfull.\n");
		
		//------------ on success fork into new process -------
		pid_t child_pid = fork();
		if (child_pid < 0){
			perror("fork");
			return -1;
		}
		if (child_pid == 0){
			//==== child process ====
			//create a new session
			int result = setsid();
			if (result < 0){
				perror("setsid");
				exit(1);
			}
			result = set_ids(user);
			if (result < 0){
				fprintf(stderr,"Could not change user ids.\n");
				exit(1);
			}
			init_env(user);
			execl("/bin/bash","/bin/bash",NULL);
			exit(0);
		}
		//----------- wait for fork to exit -------------
		int child_return;
		result = waitpid(child_pid,&child_return,0);
		if (result < 0){
			perror("waitpid");
			return -1;
		}
		int child_status = WEXITSTATUS(child_return); //the return status of the fork
		//------------ log user out when the fork exits ---------
		logout:
		pam_logout(NULL); //dont care about result
		break;
	}
	//---------------- exit cleanup ------------	
}
static int set_ids(char *user){
	struct passwd *pw = getpwnam(user);
	if (pw == NULL){
		perror("getpwnam");
		return -1;
	}
	
	//group id
	int result = setgid(pw->pw_gid);
	if (result < 0){
		perror("setgid");
		return -1;
	}
	//other non primary groups
	result = initgroups(pw->pw_name,pw->pw_gid);
	if (result < 0){
		perror("initgroups");
		return -1;
	}

	//user id
	result = setuid(pw->pw_uid);
	if (result < 0){
		perror("setgid");
		return -1;
	}

	//DOUBLE CHECK uid and euid are correct
	if ((geteuid() != pw->pw_uid) && (getuid() != pw->pw_uid)){
		fprintf(stderr,"expected uid missmatch: setting uid failed.\n");
		return -1;
	}

	//success!
	return 0;
}
static void init_env(char *username){
	//get passwd struct
	struct passwd *password = getpwnam(username);
	setenv("HOME",password->pw_dir,1);
	setenv("USER",password->pw_name,1);
	setenv("LOGNAME",password->pw_name,1);
	setenv("PWD",password->pw_dir,1);
	setenv("SHELL",password->pw_shell,1);
}
