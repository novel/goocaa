PREFIX?=/usr/local
CC=gcc
CFLAGS=-Wall -g
RM=rm -f
INSTALL=install
GOOCAA=goocaa

all:
	${CC} ${CFLAGS} `pkg-config neon libxml-2.0 glib-2.0 --libs --cflags` -o ${GOOCAA} main.c google.c

clean:
	${RM} ${GOOCAA}
	${RM} *.core

install: all
	${INSTALL} ${GOOCAA} ${PREFIX}/bin
