LFLAGS= -lpam -lpam_misc `pkg-config --libs gtk4` -lncursesw
CFLAGS=-g `pkg-config --cflags gtk4`
CC=gcc

all : microdm microdm-greeter.so
microdm : main.o pam.o graphical-interface.o start_commands.o
	$(CC) $^ -o $@ $(LFLAGS)
main.o : src/main.c src/pam.h src/start_commands.h
	$(CC) $(CFLAGS) -c src/main.c
pam.o : src/pam.c
	$(CC) $(CFLAGS) -c src/pam.c
graphical-interface.o : src/graphical-interface.c
	$(CC) $(CFLAGS) -c src/graphical-interface.c
greeter.o : src/greeter.c src/greeter.h
	$(CC) $(CFLAGS) -c -fPIC src/greeter.c
start_commands.o : src/start_commands.c
	$(CC) $(CFLAGS) -fPIC -c src/start_commands.c
microdm-greeter.so : greeter.o start_commands.o
	gcc $^ -Wl,--export-dynamic -shared -o $@ $(LFLAGS)
clean : 
	rm *.o
