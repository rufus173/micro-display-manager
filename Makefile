LFLAGS= -lpam -lpam_misc `pkg-config --libs gtk4` -lncursesw
CFLAGS=-g `pkg-config --cflags gtk4`
CC=gcc

microdm : main.o pam.o graphical-interface.o tui.o start_commands.o
	$(CC) $^ -o $@ $(LFLAGS)
main.o : src/main.c src/pam.h src/start_commands.h
	$(CC) $(CFLAGS) -c src/main.c
pam.o : src/pam.c
	$(CC) $(CFLAGS) -c src/pam.c
graphical-interface.o : src/graphical-interface.c
	$(CC) $(CFLAGS) -c src/graphical-interface.c
tui.o : src/tui.c src/tui.h
	$(CC) $(CFLAGS) -c src/tui.c
start_commands.o : src/start_commands.c
	$(CC) $(CFLAGS) -c src/start_commands.c
clean : 
	rm *.o
install : microdm microdm.service
	./install
