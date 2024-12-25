LFLAGS= -lpam -lpam_misc `pkg-config --libs gtk4`
CFLAGS=-g `pkg-config --cflags gtk4`
CC=gcc

micro-display-manager : main.o pam.o graphical-interface.o
	$(CC) $^ -o $@ $(LFLAGS)
main.o : src/main.c src/pam.h
	$(CC) $(CFLAGS) -c src/main.c
pam.o : src/pam.c
	$(CC) $(CFLAGS) -c src/pam.c
graphical-interface.o : src/graphical-interface.c
	$(CC) $(CFLAGS) -c src/graphical-interface.c
clean : 
	rm *.o
install : micro-display-manager micro-display-manager.service
	./install
