LFLAGS= -lpam -lpam_misc
CFLAGS=-g
CC=gcc

micro-display-mgr : main.o pam.o
	$(CC) $^ -o $@ $(LFLAGS)
main.o : src/main.c
	$(CC) $(CFLAGS) -c src/main.c
pam.o : src/pam.c
	$(CC) $(CFLAGS) -c src/pam.c

