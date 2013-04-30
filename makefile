# Mersy Makefile

CC=gcc
CFLAGS=-Wall -Werror -O3 -static

all: mersy main
	$(CC) $(CFLAGS) main.o mersy.o -o mersy -pthread -lgmp

main: main.c mersy.h
	$(CC) $(CFLAGS) -c main.c -o main.o

mersy: mersy.c mersy.h
	$(CC) $(CFLAGS) -c mersy.c -o mersy.o

clean:
	rm -f mersy
	rm -f *.o
