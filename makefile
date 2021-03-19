CC = gcc
CFLAGS = -g -Wall -std=gnu99

all : smallsh

smallsh : main.o smallShell.o
	$(CC) $(CFLAGS) -o $@ $^

smallShell.o : smallShell.h smallShell.c

main.o : smallShell.h main.c

clean :
	-rm *.o
	-rm smallsh
