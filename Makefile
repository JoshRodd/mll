# mll-0.2/Makefile

CC=cc
CFLAGS=-c -O4 -Oz -Ofast
PREFIX=/usr/local

all:	test

mll:	mll.o
	$(CC) mll.o -o mll

mll.o:	mll.c
	$(CC) $(CFLAGS) mll.c

install:	mll
	strip mll
	install -m 755 mll "$(PREFIX)/bin"

clean:
	rm -f mll.o mll

test:	./mll-test.sh mll
	./mll-test.sh ./mll
