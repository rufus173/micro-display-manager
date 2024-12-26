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
int tui_init(){
	setlocale(LC_ALL,""); //fixes weird characters not displaying
	initscr();
	noecho();
	refresh();
	login_window = newwin(10,10,10,10);
	box(login_window,0,0);
	printw("this is tty text. ┌─");
	wrefresh(login_window);
	getch();
	sleep(10);
	return 0;
}
int tui_end(){
	endwin();
	return 0;
}
