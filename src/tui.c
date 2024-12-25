#include <stdio.h>
#include <ncurses.h>
#include <locale.h>
int tui_init(char *tty){
	// ---------- switch stdin and stdout to the new terminal ---------
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
	setlocale(LC_ALL,""); //fixes weird characters not displaying
	initscr();
	box(stdscr,0,0);
	refresh();
	getch();
	return 0;
}
int tui_end(){
	endwin();
	return 0;
}
