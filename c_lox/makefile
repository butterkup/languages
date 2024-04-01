CC=gcc -std=c2x -O1
IPATH=./include

all: install
u: uninstall
i: install

clox: main.c
	${CC} ${CLOX_MACRO} -o $@ -I${IPATH} $^

clean:
	rm -rfv clox

uninstall:
	rm -rfv ../bin/clox

install: clox
	mv clox ../bin
	echo installed '../bin/clox'
