CC = gcc
CFLAGS = -g -Wall

program: buffer.c editor.c main.c main.o
	$(CC) $(CFLAGS) buffer.c editor.c main.c -o main.o -lncurses

clean: rm*.o program
