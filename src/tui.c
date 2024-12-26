#include <stdio.h>
#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
static int tty_fd;
static WINDOW *login_window; 
int tui_init(char *tty){
	int tty_number = 1;
	//gain controll of tty
	tty_fd = open(tty, O_RDWR); //O_NOCTTY
	if (tty_fd < 0){
		perror("open");
		return -1;
	}
	int result = ioctl(tty_fd, TIOCSCTTY); //the term now controls us

	/*// ---------- switch stdin and stdout to the new terminal ---------
	FILE *file_result;
	file_result = freopen(tty,"wb",stdout);
	if (file_result == NULL){
		perror("freopen");
		return -1;
	}
	file_result = freopen(tty,"wb",stderr);
	if (file_result == NULL){
		perror("freopen");
		return -1;
	}
	file_result = freopen(tty,"rb",stdin);
	if (file_result == NULL){
		perror("freopen");
		return -1;
	}
	file_result = NULL;
*/
	//switch to tty
	/*result = ioctl(tty_fd,VT_ACTIVATE,&tty_number);
	if (result < 0){
		perror("ioctl VT_ACTIVATE");
		return -1;
	}
	result = ioctl(tty_fd,VT_WAITACTIVE,tty_number);
	if (result < 0){
		perror("ioctl VT_ACTIVATE");
		return -1;
	}*/
	FILE *tty_out = fdopen(tty_fd,"w");
	FILE *tty_in = fdopen(tty_fd,"r");
	stdout = tty_out;
	stdin = tty_in;

	setlocale(LC_ALL,""); //fixes weird characters not displaying
	initscr();
	noecho();
	refresh();
	login_window = newwin(10,10,10,10);
	box(login_window,0,0);
	printw("this is tty text. ┌─");
	wrefresh(login_window);
	getch();
	return 0;
}
int tui_end(){
	endwin();
	close(tty_fd);
	int result = vhangup(); //relinquish our terminal
	if (result < 0){
		perror("vhangup");
		return -1;
	}
	return 0;
}
