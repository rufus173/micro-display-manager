#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <pwd.h>

#define MAX(n1,n2) ((n1 > n2) ? n1 : n2 )
#define CLAMP_MIN(n,min) ((n > min) ? n : min)
#define CLAMP_MAX(n,max) ((n < max) ? n : max)
#define MAX_LABEL_WIDTH 16

void print_centred(WINDOW *window,int y,int x,int max_length,char *text);
int get_start_commands(char ***all_start_commands);
void free_start_commands(char **all_start_commands,int max_start_commands);

static int tty_fd;
int tui_init(){
	setlocale(LC_ALL,""); //fixes weird characters not displaying
	initscr();
	noecho();
	cbreak();
	curs_set(0);
	keypad(stdscr, TRUE);
	refresh();
	return 0;
}
int tui_end(){
	endwin();
	return 0;
}
int tui_get_user_and_password(char **user, char **password, char **start_command){
	//======== prep ==========
	//get required info
	char **all_users = NULL;
	int user_count = 0;
	int max_username_len = 0;
	setpwent();
	for (struct passwd *pw = getpwent();pw != NULL;pw = getpwent()){
		if (pw->pw_uid < 1000 || pw->pw_uid > 60000) continue; //dont show system users
		user_count++;
		all_users = realloc(all_users,user_count*sizeof(char *));
		all_users[user_count-1] = strdup(pw->pw_name);
		max_username_len = MAX(max_username_len,strlen(pw->pw_name));
	}
	endpwent();
	
	char **all_start_commands;
	int max_start_commands = get_start_commands(&all_start_commands);
	
	//ui
	int window_width = 2;
	int window_height = 5;
	int window_x = (COLS/2)-(window_width/2);
	int window_y = (LINES/2)-(window_height/2);
	WINDOW *login_window = newwin(window_height,window_width,window_y,window_x);
	//user data
	int selected_user = 0;
	int selected_start_command = 0;
	int entered_password_size = 1;
	char *entered_password = malloc(1);
	entered_password[0] = '\0';
	//======== mainloop =======
	for (;;){
		char **start_commands = NULL;
		int start_command_count = 0;
		// --------------- render window -------------
		werase(login_window);
		clear();
		refresh();
		
		//controls
		printw("left and right arrows: change user\nup and down arrows: change start command");

		//regenerate required information to render
		window_width = 2;
		window_width = MAX(window_width,max_username_len+4+MAX_LABEL_WIDTH); //4 for padding and MAX_LABEL_WIDTH for labels
		window_width = MAX(window_width,entered_password_size+3+MAX_LABEL_WIDTH);
		window_width = MAX(window_width,strlen(all_start_commands[selected_start_command])+4+MAX_LABEL_WIDTH);
		window_width = CLAMP_MAX(window_width,COLS);
		window_x = (COLS/2)-(window_width/2);
		
		mvwin(login_window,window_y,window_x);
		wresize(login_window,window_height,window_width);
		box(login_window,0,0);

		//user label
		mvwprintw(login_window,1,2,"User          |");
		//user
		print_centred(login_window,1,2+MAX_LABEL_WIDTH,window_width-4-MAX_LABEL_WIDTH,all_users[selected_user]);
		//password label
		mvwprintw(login_window,2,2,"Password      |");
		//pasword (just displayed as ***** with the coresponding length);
		for (int i = 0; i < entered_password_size-1; i++) mvwprintw(login_window,2,MAX_LABEL_WIDTH+2+i,"*");
		//start command label
		mvwprintw(login_window,3,2,"Start command |");
		//start command
		print_centred(login_window,3,2+MAX_LABEL_WIDTH,window_width-4-MAX_LABEL_WIDTH,all_start_commands[selected_start_command]);

		


		wrefresh(login_window);
		refresh();
		//--------------- input processing -------------
		int input = getch();
		//mvprintw(0,0,"%d     %d     %d",input,KEY_LEFT,KEY_RIGHT);
		switch (input){
			case KEY_LEFT:
				selected_user--;
				if (selected_user < 0) selected_user = user_count-1; //wrap back to the begining
				break;
			case KEY_RIGHT:
				selected_user++;
				if (selected_user > user_count-1) selected_user = 0;
				break;
			case KEY_UP:
				selected_start_command++;
				if (selected_start_command >= max_start_commands) selected_start_command = 0;
				break;
			case KEY_DOWN:
				selected_start_command--;
				if (selected_start_command < 0) selected_start_command = max_start_commands-1;
				break;
			case KEY_BACKSPACE:
				entered_password_size--;
				if (entered_password_size < 1) entered_password_size = 1;
				entered_password[entered_password_size-1] = '\0';
				break;
			case '\n':
				*user = strdup(all_users[selected_user]);
				*password = strdup(entered_password);
				*start_command = strdup(all_start_commands[selected_start_command]);
				return 0;
			default:
				if (input < 32 || input > 126) break;
				entered_password_size++;
				entered_password = realloc(entered_password,entered_password_size);
				entered_password[entered_password_size-2] = (char)input;
				entered_password[entered_password_size-1] = '\0';
		}
	}
	//====== cleanup =======
	for (int i = 0; i < user_count; i++) free(all_users[user_count]);
	free(all_users);
	free_start_commands(all_start_commands,max_start_commands);
	return 0;
}
void print_centred(WINDOW *window,int y,int x,int max_length,char *text){
	//find the centre
	int centred_x = x+(max_length/2);
	
	//align with the centre of the text
	centred_x -= strlen(text)/2;

	//make sure it doesnt overspill
	centred_x = CLAMP_MAX(centred_x,x+max_length-strlen(text));
	centred_x = CLAMP_MIN(centred_x,x);

	//actualy print
	mvwprintw(window,y,centred_x,"%s",text);
}
int get_start_commands(char ***all_start_commands){
	int max_start_commands = 3;
	char **start_commands = malloc(sizeof(char *)*max_start_commands);
	start_commands[0] = strdup("/bin/bash");
	start_commands[1] = strdup("startx");
	start_commands[2] = strdup("startplasma-wayland");

	*all_start_commands = start_commands;
	return max_start_commands;
}
void free_start_commands(char **all_start_commands,int max_start_commands){
	for (int i = 0; i < max_start_commands; i++) free(all_start_commands[i]);
	free(all_start_commands);
}
