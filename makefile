# Mersy Makefile

CC=gcc
CFLAGS=-Wall -Werror -O3 -march=corei7-avx -mfpmath=sse -mavx -static

all: mersy

mersy: mersy.c
	$(CC) $(CFLAGS) mersy.c -o mersy -pthread -lgmp

clean:
	rm -f mersy
