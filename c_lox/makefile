CC=gcc -std=c2x
IPATH=./include

all: install

clox: main.c
	${CC} -o $@ -I${IPATH} $^

clean:
	rm -rfv clox

uninstall:
	rm -rfv ../bin/clox

u: uninstall

install: clox
	mv clox ../bin
	echo installed '../bin/clox'
