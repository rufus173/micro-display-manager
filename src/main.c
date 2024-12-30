#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pam.h"
#include <wait.h>
#include "graphical-interface.h"
#include <pwd.h>
#include <grp.h>
#include "tui.h"
#include "start_commands.h"
static void set_xdg_env();
static void init_env(char *username);
static int set_ids(char *user);

int main(int argc, char **argv){
	if (load_desktops() < 0){
		fprintf(stderr,"could not load available desktops\n");
		return 1;
	}
	if (getuid() != 0){
		fprintf(stderr,"Must be run as root.\n");
		return -1;
	}

	//ignore sigint to stop CTRL-c killing the display manager
	signal(SIGINT,SIG_IGN);

	//-------- mainloop to allow more then one login and logout ---------
	for (;;){
		//------------ get username and password -------------
		//start the tui
		int result = tui_init();
		if (result < 0){
			fprintf(stderr,"tui would not start.\n");
			return 1;
		}


		char *user = NULL;
		char *password = NULL;
		char *start_command = NULL;
		result = tui_get_user_and_password(&user,&password,&start_command);//allocates strings for us

		//clean up after the tui
		tui_end();
		//printf("user: [%s]\npassword: [%s]\nstart command: [%s]\n",user,password,start_command);

		//------------ check credentials with pam ------------
		printf("verifying credentials\n");
		//we need pam_systemd.so to trigger to set up a new session for us
		set_xdg_env();
		result = pam_login(user,password,NULL); //switch to desired user
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
			execl("/bin/bash","bash","-i",NULL);
			execl("/bin/sh","sh","-c","/usr/lib/plasma-dbus-run-session-if-needed /usr/bin/startplasma-wayland",NULL);
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
		if (result >= 0) pam_logout(NULL); //dont logout if login failed
		free(user); free(password); free(start_command);
		break;
		//sleep(5);
	}
	//---------------- exit cleanup ------------	
	free_desktops();
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
	//setenv("LANG","en_US.UTF-8",1);
}
static void set_xdg_env(){
	setenv("XDG_SESSION_TYPE","tty",1);
	setenv("XDG_SEAT","seat0",1);
	setenv("XDG_SESSION_CLASS","user",1);
	setenv("XDG_VTNR","1",1);
}
