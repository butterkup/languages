CC=gcc -std=c2x -O0
IPATH=./include

all: install

clox: main.c
	${CC} ${CLOX_MACRO} -o $@ -I${IPATH} $^

clean:
	rm -rfv clox

uninstall:
	rm -rfv ../bin/clox

u: uninstall

install: clox
	mv clox ../bin
	echo installed '../bin/clox'

i: install
