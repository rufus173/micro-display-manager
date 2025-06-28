#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <errno.h>
#include <math.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <pwd.h>
#include "start_commands.h"
#include <poll.h>

#define MAX(n1,n2) ((n1 > n2) ? n1 : n2 )
#define CLAMP_MIN(n,min) ((n > min) ? n : min)
#define CLAMP_MAX(n,max) ((n < max) ? n : max)
#define MAX_LABEL_WIDTH 16

struct available_desktops *desktops = NULL;

void print_centred(WINDOW *window,int y,int x,int max_length,char *text);
int get_start_commands(char ***all_start_commands);
void free_start_commands(char **all_start_commands,int max_start_commands);
void print_boxed(WINDOW *window,int y,int x,int width,int height,const char *text);
int redirect_stderr();
int unredirect_stderr();
char *format(char *format, ...);
int async_loop();

WINDOW *stderr_output_window;

int stderr_old_fd;
int stderr_pipe_out;
int stderr_pipe_in;

void *greeter_init(struct available_desktops *available_desktops){
	redirect_stderr();
	setlocale(LC_ALL,""); //fixes weird characters not displaying
	initscr();
	stderr_output_window = newwin(5,COLS,LINES-5,0);
	scrollok(stderr_output_window,TRUE);
	noecho();
	cbreak();
	curs_set(0);
	keypad(stdscr, TRUE);
	refresh();
	desktops = available_desktops;
	//option to return a state that will be passed back to all the functions but i cba so globals it is
	return (void *)1;
}
int greeter_end(void *state){
	delwin(stderr_output_window);
	endwin();
	desktops = NULL;
	unredirect_stderr();
	return 0;
}
int greeter_get_login_info(void *state,char **user, char **password, int *desktop_index){
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
	
	int max_start_commands = get_desktop_count(desktops); //bad variable naming (sorry)
	
	//ui
	int window_width = 2;
	int window_height = 5;
	int window_x = (COLS/2)-(window_width/2);
	int window_y = (LINES/2)-(window_height/2);
	WINDOW *login_window = newwin(window_height,window_width,window_y,window_x);
	//user data
	int selected_user = 0;
	int selected_start_command = get_last_selected_desktop_index(desktops);
	int entered_password_size = 1;
	char *entered_password = malloc(1);
	entered_password[0] = '\0';
	//======== mainloop =======
	for (;;){
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
		window_width = MAX(window_width,strlen(get_desktop_name(desktops,selected_start_command))+4+MAX_LABEL_WIDTH);
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
		print_centred(login_window,3,2+MAX_LABEL_WIDTH,window_width-4-MAX_LABEL_WIDTH,get_desktop_name(desktops,selected_start_command));

		


		wrefresh(login_window);
		refresh();
		//--------------- input processing -------------
		for (;;){
			//=== check if data available on stdin ===
			struct pollfd fds = {
				.fd = STDIN_FILENO,
				.events = POLLIN
			};
			int result = poll(&fds,1,0);
			if (result < 0){
				perror("poll");
				break;
			}
			//run the update loop if no input available
			if (!fds.revents & POLLIN) async_loop();
			//continue the input loop if data available
			else break;
		}
		//=== process inputs if they are available ===
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
				*desktop_index = selected_start_command;
				set_last_selected_desktop_index(desktops,selected_start_command);
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
	delwin(login_window);
	return 0;
}
void print_boxed(WINDOW *window,int y,int x,int width,int height,const char *text){
	int current_y = y;
	int current_x = x;
	for (int i = 0; i < strlen(text); i++){
		mvwprintw(window,current_y,current_x,"%c",text[i]);
		current_x++;
		if (current_x >= width+x){
			current_y++;
			current_x = x;
		}
		if (current_y >= height+y) break;
	}
}
int show_message(const char *title,const char *message){
	int max_length = MAX(COLS/3,strlen(title)+2);
	int width = max_length+4;
	int height = ceil((double)strlen(message)/(double)max_length)+2;
	WINDOW *message_window = newwin(height,width,LINES/2-height/2,COLS/2-width/2);
	box(message_window,0,0);
	mvwprintw(message_window,0,1,"%s",title);
	print_boxed(message_window,1,1,width-2,height-2,message);
	wrefresh(message_window);
	getch();
	delwin(message_window);
	return 0;
}
int greeter_show_error(void *handle,const char *error){
	show_message("Error",error);
	return 0;
}
int greeter_show_info(void *handle,const char *message){
	show_message("Info",message);
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
int redirect_stderr(){
	stderr_old_fd = dup(fileno(stderr));
	int filedes[2];
	if(pipe(filedes) != 0){
		return errno;
	}
	stderr_pipe_out = filedes[0];
	stderr_pipe_in = filedes[1];
	char *new_stderr_filedes = format("/proc/self/fd/%d",stderr_pipe_in);
	if (freopen(new_stderr_filedes,"w",stderr) == NULL){
		free(new_stderr_filedes);
		return errno;
	}
	//change output to non blocking
	int flags = fcntl(stderr_pipe_out,F_GETFL);
	if (flags < 0){
		perror("fcntl");
	}
	int result = fcntl(stderr_pipe_out,F_SETFL,flags | O_NONBLOCK);
	if (result < 0){
		perror("fcntl");
	}
	free(new_stderr_filedes);
	return 0;
}
int unredirect_stderr(){
	char *old_fd_location = format("/proc/self/fd/%d",stderr_old_fd);
	if (freopen(old_fd_location,"w",stderr) == NULL){
		printf("freopen - unredirect_stderr: %s\n",strerror(errno));
		free(old_fd_location);
		return errno;
	}
	free(old_fd_location);
	//print any backlog left on the pipe
	close(stderr_pipe_in);
	for (;;){
		char buffer[24];
		ssize_t size = read(stderr_pipe_out,buffer,sizeof(buffer));
		if (size == 0) break; //done
		if (size < 0){
			perror("read");
			close(stderr_pipe_out);
			return -1;
		}
		fwrite(buffer,1,size,stderr);
		fflush(stderr);
	}
	close(stderr_pipe_out);
	return 0;
}
char *format(char *format, ...){
	va_list args;
	va_list duped_args;
	va_start(args,format);
	//the first vsnprintf "consumes" args, so we need a second for the second call
	va_copy(duped_args,args);
	//vsnprintf then malloc then vsnprintf
	size_t size = vsnprintf(NULL,0,format,args);
	char *formatted_string = malloc(size+1); //dont forget about the terminating null
	size = vsnprintf(formatted_string,size+1,format,duped_args);
	va_end(args);
	va_end(duped_args);
	return formatted_string;
}
int async_loop(){
	fprintf(stderr,"sample stderr output\n");
	char buffer[1024];
	errno = 0;
	ssize_t count = read(stderr_pipe_out,buffer,sizeof(buffer));
	if (count < 0){
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){
			//skip as no data available on pipe
		}else{
			perror("read");
			return -1;
		}
	}
	if (count > 0){
		//print out the pipe contents
		for (ssize_t i = 0; i < count; i++){
			waddch(stderr_output_window,buffer[i]);
		}
		wrefresh(stderr_output_window);
	}
	return 0;
}
