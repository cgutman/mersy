# Mersy Makefile

CC=gcc
CFLAGS=-Wall -Werror -O3 -static

all: mersy mersy-impl
	$(CC) $(CFLAGS) mersy-impl.o mersy.o -o mersy -pthread -lgmp

mersy-impl: mersy-impl.c mersy.h mersy-impl.h
	$(CC) $(CFLAGS) -c mersy-impl.c -o mersy-impl.o

mersy: mersy.c mersy.h mersy-impl.h
	$(CC) $(CFLAGS) -c mersy.c -o mersy.o

clean:
	rm -f mersy
	rm -f *.o
