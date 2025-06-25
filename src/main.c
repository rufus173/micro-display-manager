#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pam.h"
#include <wait.h>
#include "graphical-interface.h"
#include <pwd.h>
#include <grp.h>
#include "start_commands.h"
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <dlfcn.h>
static void set_xdg_env();
static void init_env(char *username);
static int set_ids(char *user);
static void *(*greeter_init)(struct available_desktops *);
static int (*greeter_end)(void *);
static int (*greeter_get_login_info)(void *,char**,char**,int *);
static int (*greeter_show_error)(void *,char *);

int main(int argc, char **argv){
	if (getuid() != 0){
		fprintf(stderr,"Must be run as root.\n");
		return 1;
	}
	struct available_desktops *desktops;
	if ((desktops = load_desktops()) < 0){
		fprintf(stderr,"could not load available desktops\n");
		return 1;
	}
	//------------ open the greeter shared object ------------
	void *greeter_handle = dlopen("./microdm-greeter.so",RTLD_LAZY);
	if (greeter_handle == NULL){
		printf("dlopen: %s\n",dlerror());
		return 1;
	}
	//=== load all the functions ===
	if ((greeter_init = dlsym(greeter_handle,"greeter_init")) == NULL){
		fprintf(stderr,"greeter has not exported greeter_get_login_info symbol");
		return 1;
	}
	if ((greeter_get_login_info = dlsym(greeter_handle,"greeter_get_login_info")) == NULL){
		fprintf(stderr,"greeter has not exported greeter_get_login_info symbol");
		return 1;
	}
	if ((greeter_end = dlsym(greeter_handle,"greeter_end")) == NULL){
		fprintf(stderr,"greeter has not exported greeter_end symbol");
		return 1;
	}
	if ((greeter_show_error = dlsym(greeter_handle,"greeter_show_error")) == NULL){
		fprintf(stderr,"greeter has not exported greeter_show_error symbol");
		return 1;
	}
	void *context = greeter_init(load_desktops());
	for (;;){
		char *user,*password;
		int desktop_entry;
		greeter_get_login_info(context,&user,&password,&desktop_entry);
		greeter_show_error(context,"this is a sample of an error message");
		free(user);
		free(password);
	}
	greeter_end(context);

	//ignore sigint to stop CTRL-c killing the display manager
	//signal(SIGINT,SIG_IGN);

	//-------- mainloop to allow more then one login and logout ---------
	//we need pam_systemd.so to trigger to set up a new session for us
	set_xdg_env();
	void *greeter_context = NULL;
	for (;;){
		//------------ start tui ------------
		greeter_context = greeter_init(desktops);
		if (greeter_context == NULL){
			fprintf(stderr,"greeter failed to load");
			break;
		}
		//------------ attempt to login ------------
		char *user = NULL;
		char *password = NULL;
		int desktop_index = 0;

		pam_handle_t *login_handle;
		for (;;){
			//------ repeatedly get password untill one works ------
			greeter_get_login_info(greeter_context,&user,&password,&desktop_index);
			int result = pam_login(user,password,&login_handle); //switch to desired user
			if (result == PAM_SUCCESS) break;
			free(user);
			free(password);
			switch(result){
				case PAM_AUTH_ERR:
				greeter_show_error(greeter_context,"access denied :3");
				break;
				case PAM_MAXTRIES:
				greeter_show_error(greeter_context,"maximum tries exceded :(");
				break;
				case PAM_AUTHINFO_UNAVAIL:
				greeter_show_error(greeter_context,"unable to access auth information");
				break;
				default:
				greeter_show_error(greeter_context,"unknown error");
				break;
			}
		}
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
		int result = waitpid(child_pid,&child_return,0);
		if (result < 0){
			perror("waitpid");
			return -1;
		}
		int child_status = WEXITSTATUS(child_return); //the return status of the fork
		//------------ logout of pam ------------
		free(user);
		free(password);
		pam_logout(login_handle);
	}
	//---------------- exit cleanup ------------	
	if (dlclose(greeter_handle) != 0){
		printf("dlclose: %s\n",dlerror());
		return 1;
	}
	free_desktops(desktops);
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
