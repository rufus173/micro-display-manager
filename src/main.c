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
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <dlfcn.h>
static void set_xdg_env();
static void init_env(char *username);
static int set_ids(char *user);

int main(int argc, char **argv){
	if (getuid() != 0){
		fprintf(stderr,"Must be run as root.\n");
		return -1;
	}
	if (load_desktops() < 0){
		fprintf(stderr,"could not load available desktops\n");
		return 1;
	}
	//------------ open the greeter shared object ------------
	void *greeter_handle = dlopen("microdm-greeter.so",RTLD_LAZY);
	if (greeter_handle == NULL){
		printf("dlopen: %s\n",dlerror());
	}
	//return 0;

	//ignore sigint to stop CTRL-c killing the display manager
	//signal(SIGINT,SIG_IGN);

	//-------- mainloop to allow more then one login and logout ---------
	//we need pam_systemd.so to trigger to set up a new session for us
	set_xdg_env();
	void *greeter_context = NULL;
	for (;;){
		//------------ start tui ------------
		greeter_context = greeter_init();
		//------------ attempt to login ------------
		char *user = NULL;
		char *password = NULL;
		int desktop_index = 0;
		greeter_get_login_info(greeter_context,&user,&password,&desktop_index);

		pam_handle_t *login_handle;
		printf("verifying credentials\n");
		int result = pam_login(user,password,&login_handle); //switch to desired user
		printf("pam login successfull.\n");
		if (result < 0){
			fprintf(stderr,"could not log in\n");
			greeter_show_error("Login incorrect");
		}else{
			//------------ on success fork into new process -------------
			pid_t child_pid = fork();
			if (child_pid < 0){
				perror("fork");
				return -1;
			}
			//==== child process ====
			if (child_pid == 0){
				//create a new session
				int result = setsid();
				if (result < 0){
					perror("setsid");
				}
				result = set_ids(user);
				if (result < 0){
					fprintf(stderr,"Could not change user ids.\n");
					exit(1);
				}
				init_env(user);
				execl("/usr/bin/sh","-sh","-i",NULL);
			}
			//----------- wait for fork to exit -------------
			int child_return;
			result = waitpid(child_pid,&child_return,0);
			if (result < 0){
				perror("waitpid");
				return -1;
			}
			int child_status = WEXITSTATUS(child_return); //the return status of the fork
			//------------ logout of pam ------------
			pam_logout(login_handle);
		}
		free(user);
		free(password);
	}
	//---------------- exit cleanup ------------	
	if (dlclose(greeter_handle) != 0){
		printf("dlclose: %s\n",dlerror())
		return 1;
	}
	free_desktops();
	greeter_end(greeter_context);
	greeter_context = NULL;
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
