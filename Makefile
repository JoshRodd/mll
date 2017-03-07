# mll-0.2/Makefile

CC=cc
CFLAGSO=-O4 -Oz -Ofast
CFLAGS=-g -O0
PREFIX=/usr/local

all:	test mll Makefile

mll:	mll.c Makefile
	$(CC) $(CFLAGS) mll.c -o mll

bin/mll:	mll.c Makefile
	mkdir -p bin
	$(CC) $(CFLAGSO) mll.c -o bin/mll
	strip bin/mll

install:	bin/mll test Makefile
	install -m 755 bin/mll "$(PREFIX)/bin"

commit:	test clean Makefile
	git commit

clean:
	rm -f mll mll.o
	rm -rf mll.dSYM bin

test:	./mll-test.sh mll bin/mll Makefile
	./mll-test.sh mll
	./mll-test.sh bin/mll
