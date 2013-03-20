# Mersy Makefile

CC=gcc
CFLAGS=-Wall -Werror -O3 -static

all: mersy

mersy: mersy.c
	$(CC) $(CFLAGS) mersy.c -o mersy -pthread -lgmp

clean:
	rm -f mersy
